export module Katline:Runtime;

import CommonLib;
import :FramebufferController;
import :Thread;
import :Logo;
import :SerialController;
import :MemoryData;
import :CPU;
import :Scheduler;
import :Interrupts;
import :X2APIC;
import :FrameAllocator;
import :Paging;
import :MemoryManager;

export {
	namespace Katline {

	struct StartupInfo {
		Controller::Framebuffer *framebuffer;
		Memory::MemoryMap *mmap;
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

namespace Katline {

static auto reserve_mmap_phys_range(Memory::MemoryMap *mmap, uptr hhdm_offset,
    uptr phys_base, usize size) -> void
{
	for (u64 i {}; i < mmap->size; ++i) {
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

	Interrupts::init_defaults();
	auto const timer_init_result {
		Arch::X2APIC::init_timer(info.tsc_frequency_hz),
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
	auto const stack_top { (rsp + 4095ull) & ~4095ull };
	auto const stack_start {
		stack_top > info.stack_size ? stack_top - info.stack_size : 0
	};
	for (uptr virt { stack_start }; virt < stack_top; virt += 4096ull) {
		auto const phys { Arch::Paging::translate(current_root, virt) };
		if (phys) {
			reserve_mmap_phys_range(info.mmap, info.hhdm_offset, *phys, 4096);
			Memory::FA::reserve_phys_range(*phys, 4096);
		}
	}
	Memory::MM::init(info.mmap);
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

	CL::ArrayList<int> numbers { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	CL::ignore_unused(numbers);

	auto const transformed {
		CL::range(10)
		    .map([](int value) { return value * 2; })
		    .filter([](int value) { return value >= 6; })
		    .rev()
		    .collect<CL::ArrayList<int>>(),
	};

	Debug::print_formatted("[demo] arraylist: ");
	transformed.iter().for_each(
	    [](int value) { Debug::print_formatted("%d ", value); });
	Debug::print_formatted("\n");

	CL::HashMap<int, int> squares;
	squares.insert_or_replace(2, 4);
	squares.insert_or_replace(3, 9);

	Debug::print_formatted("[demo] hashmap: ");
	squares.iter().for_each([](auto const &entry) {
		Debug::print_formatted("(%d->%d) ", entry.key, entry.value);
	});
	Debug::print_formatted("\n");

	k_bsp_initialized.store(true, CL::MemoryOrder::Release);
	Debug::drain_logs();

	u64 last_logged {};
	for (;;) {
		Debug::drain_logs();

		u64 ticks { Arch::X2APIC::timer_ticks() };
		u64 cur { ticks / 250ull };
		if (cur != 0 && cur != last_logged) {
			last_logged = cur;
			Debug::print_formatted("[x2APIC] tick=%d (~%ds)\n",
			    static_cast<int>(ticks), static_cast<int>(cur));
			Debug::drain_logs();
		}

		if (ticks >= 500) {
			Debug::print_formatted("[demo] triggering deliberate page fault\n");
			*reinterpret_cast<u64 volatile *>(0) = 0xdeadbeefull;
		}
	}
}

void boot_cpu(u32 lapic_id, u32 processor_id, u64 extra)
{
	// wait for bsp init
	while (!k_bsp_initialized.load(CL::MemoryOrder::Acquire))
		asm volatile("pause");

	CL::ignore_unused(lapic_id, processor_id, extra);

	print_cpu_info(lapic_id);

	asm volatile("cli");
	for (;;)
		asm volatile("hlt");
}

}
