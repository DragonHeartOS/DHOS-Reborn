export module Katline:Runtime;

import CommonLib;
import :FramebufferController;
import :Thread;
import :Logo;
import :SerialController;
import :ArchConstants;
import :MemoryData;
import :CPU;
import :GDT;
import :Scheduler;
import :Interrupts;
import :X2APIC;
import :FrameAllocator;
import :Paging;
import :MemoryManager;
import :HandleManager;
import :SyscallABI;
import :Bootstrap;

export {
	namespace Katline {

	struct StartupInfo {
		Controller::Framebuffer *framebuffer;
		Memory::MemoryMap *mmap;
		u32 cpu_count;
		u32 bsp_lapic_id;
		uptr rsdp_address;
		uptr hhdm_offset;
		u64 tsc_frequency_hz;
		u64 stack_size;
		u64 kernel_physical_base;
		u64 kernel_size;
	};

	extern Controller::FramebufferController k_framebuffer_controller;
	extern Controller::SerialController k_serial_controller;

	void katline_main(StartupInfo &info);
	void boot_cpu(u32 lapic_id, u32 processor_id, u64 extra);

	}
}

namespace Katline::Debug {
void set_framebuffer_logging_enabled(bool enabled);
auto drain_logs() -> void;
void print_formatted(char const *str, ...);
}

extern "C" auto memcpy(void *dst, void const *src, usize size) -> void *
{
	auto *d = static_cast<unsigned char *>(dst);
	auto const *s = static_cast<unsigned char const *>(src);
	for (usize i {}; i < size; ++i)
		d[i] = s[i];
	return dst;
}

extern "C" auto memmove(void *dst, void const *src, usize size) -> void *
{
	auto *d = static_cast<unsigned char *>(dst);
	auto const *s = static_cast<unsigned char const *>(src);
	if (d == s || size == 0)
		return dst;
	if (d < s) {
		for (usize i {}; i < size; ++i)
			d[i] = s[i];
	} else {
		for (usize i = size; i-- > 0;)
			d[i] = s[i];
	}
	return dst;
}

extern "C" auto memset(void *dst, int value, usize size) -> void *
{
	auto *d = static_cast<unsigned char *>(dst);
	unsigned char const byte = static_cast<unsigned char>(value);
	for (usize i {}; i < size; ++i)
		d[i] = byte;
	return dst;
}

extern "C" auto memcmp(void const *lhs, void const *rhs, usize size) -> int
{
	auto const *a = static_cast<unsigned char const *>(lhs);
	auto const *b = static_cast<unsigned char const *>(rhs);
	for (usize i {}; i < size; ++i) {
		if (a[i] != b[i])
			return static_cast<int>(a[i]) - static_cast<int>(b[i]);
	}
	return 0;
}

extern "C" auto strlen(char const *str) -> usize
{
	usize len {};
	if (str == nullptr)
		return 0;
	while (str[len] != '\0')
		++len;
	return len;
}

namespace Katline {

static auto reserve_mmap_phys_range(Memory::MemoryMap *mmap, uptr hhdm_offset,
    uptr phys_base, usize size) -> void
{
	for (u64 i {}; i < mmap->size; i++) {
		auto *const md { &mmap->data[i] };
		if (md->type != Memory::MemoryType::USABLE || md->size == 0)
			continue;

		auto const entry_phys_base { md->base - hhdm_offset };
		if (CL::Memory::ranges_overlap(entry_phys_base,
		        static_cast<uptr>(md->size), phys_base,
		        static_cast<uptr>(size)))
			md->type = Memory::MemoryType::RESERVED;
	}
}

Controller::FramebufferController k_framebuffer_controller { nullptr };
Controller::SerialController k_serial_controller;
CL::Atomic<bool> k_bsp_initialized { false };
static constexpr usize max_cpu_slots { 256 };
static Arch::GDT s_gdt_by_lapic_id[max_cpu_slots] {};
static Arch::TSS s_tss_by_lapic_id[max_cpu_slots] {};

static auto load_cpu_segments(u32 lapic_id) -> void
{
	auto &gdt { s_gdt_by_lapic_id[lapic_id] };
	auto &tss { s_tss_by_lapic_id[lapic_id] };
	gdt.load(lapic_id, tss);
}

auto print_cpu_info(u32 lapic_id)
{
	auto const cpuid_info { Arch::CPUID::query_cpuid() };
	Debug::print_formatted(
	    "[CPU%d] vendor=%s family=%d model=%d stepping=%d apic=%d x2apic=%d\n",
	    lapic_id, cpuid_info.vendor_id,
	    static_cast<int>(cpuid_info.family + cpuid_info.ext_family),
	    static_cast<int>((cpuid_info.ext_model << 4) | cpuid_info.model),
	    static_cast<int>(cpuid_info.stepping_id),
	    static_cast<int>(cpuid_info.has_apic),
	    static_cast<int>(cpuid_info.has_x2apic));
}

auto katline_main(StartupInfo &info) -> void
{
	Debug::init_log_queue();
	k_serial_controller.init();

	Debug::drain_logs();
	k_framebuffer_controller
	    = Controller::FramebufferController(info.framebuffer);
	k_framebuffer_controller.put_logo(
	    Logo::Data, Logo::Width, Logo::Height, 10, 10);
	Debug::set_framebuffer_logging_enabled(true);

	auto const cpuid_info { Arch::CPUID::query_cpuid() };
	print_cpu_info(info.bsp_lapic_id);
	Debug::drain_logs();

	if (!cpuid_info.has_apic || !cpuid_info.has_x2apic) {
		kpanic("[CPU] missing APIC/x2APIC support; halting for bring-up.\n");
	}

	load_cpu_segments(info.bsp_lapic_id);
	Interrupts::init_defaults(info.bsp_lapic_id);
	Syscalls::init();
	auto const timer_init_result {
		Arch::X2APIC::init_local_timer(info.tsc_frequency_hz),
	};
	if (timer_init_result.is_err()) {
		auto const &error { timer_init_result.unwrap_err() };
		Debug::print_formatted("[x2APIC] timer init failed: %s\n",
		    Arch::X2APIC::error_to_string(error).data());
		kpanic("[x2APIC] timer init failed.");
	}
	Debug::drain_logs();

	Memory::FA::init(info.mmap, info.hhdm_offset);
	Arch::Paging::init(info.hhdm_offset);
	reserve_mmap_phys_range(info.mmap, info.hhdm_offset, 0, 0x100000);
	Memory::FA::reserve_phys_range(
	    info.kernel_physical_base, static_cast<usize>(info.kernel_size));
	reserve_mmap_phys_range(info.mmap, info.hhdm_offset,
	    static_cast<uptr>(info.kernel_physical_base),
	    static_cast<usize>(info.kernel_size));

	auto *const current_root { Arch::Paging::current_root() };
	auto const rsp { Arch::current_rsp() };
	auto const stack_top {
		(rsp + Arch::k_page_size - 1) & ~Arch::k_page_mask,
	};
	auto const stack_start {
		stack_top > info.stack_size ? stack_top - info.stack_size : 0
	};
	for (uptr virt { stack_start }; virt < stack_top;
	    virt += Arch::k_page_size) {
		auto const phys { Arch::Paging::translate(current_root, virt) };
		if (phys) {
			reserve_mmap_phys_range(
			    info.mmap, info.hhdm_offset, *phys, Arch::k_page_size);
			Memory::FA::reserve_phys_range(*phys, Arch::k_page_size);
		}
	}
	Memory::MM::init(info.mmap);
	Arch::HandleManager::the();
	auto *const shadow_root { Arch::Paging::clone_current_address_space() };
	if (!shadow_root) {
		Debug::print_formatted(
		    "[Paging] failed to clone current address space\n");
		for (;;)
			asm("hlt");
	}
	Arch::load_cr3(Arch::Paging::phys_address(shadow_root));
	Debug::drain_logs();

	Arch::Scheduler::the().init();
	Arch::Scheduler::the().adopt_current_thread(
	    Arch::k_process, stack_start, info.stack_size);

	asm volatile("sti");

	k_bsp_initialized.store(true, CL::MemoryOrder::Release);
	while (Arch::Scheduler::the().online_cpu_count() < info.cpu_count)
		asm volatile("pause");
	Debug::drain_logs();
	launch_bootstrap_process();
	Arch::Scheduler::the().yield();

	u64 last_logged {};
	for (;;) {
		u64 ticks { Arch::X2APIC::timer_ticks() };
		u64 cur { ticks / 50ull };
		if (cur != 0 && cur != last_logged) {
			last_logged = cur;
			Debug::drain_logs();
		}
	}
}

void boot_cpu(u32 lapic_id, u32 processor_id, u64 extra, u64 tsc_freq)
{
	// wait for bsp init
	while (!k_bsp_initialized.load(CL::MemoryOrder::Acquire))
		asm volatile("pause");

	CL::ignore_unused(processor_id, extra);

	print_cpu_info(lapic_id);
	load_cpu_segments(lapic_id);
	Interrupts::init_defaults(lapic_id);
	Syscalls::init();
	auto const timer_init_result {
		Arch::X2APIC::init_local_timer(tsc_freq),
	};
	if (timer_init_result.is_err()) {
		auto const &error { timer_init_result.unwrap_err() };
		Debug::print_formatted("[CPU%d] x2APIC init failed: %s\n", lapic_id,
		    Arch::X2APIC::error_to_string(error).data());
		for (;;)
			asm volatile("hlt");
	}

	Arch::Scheduler::the().init();
	auto const rsp { Arch::current_rsp() };
	auto const stack_top { (rsp + Arch::k_page_size - 1) & ~Arch::k_page_mask };
	auto const stack_start { stack_top > 65536ull ? stack_top - 65536ull : 0 };
	Arch::Scheduler::the().adopt_current_thread(
	    Arch::k_process, stack_start, 65536ull);

	asm volatile("sti");
	for (;;)
		asm volatile("hlt");
}

}
