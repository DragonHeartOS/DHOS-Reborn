module;

export module Katline:Runtime;

import CommonLib;
import KatlineAPI;
import :FramebufferController;
import :Thread;
import :Logo;
import :SerialController;
import :ArchConstants;
import :MemoryData;
import :MemoryObject;
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

	void katline_main(StartupInfo &info, BootData const &boot);
	void boot_cpu(u32 lapic_id, u32 processor_id, u64 extra);
	auto kernel_handle_ipc_message(IPC::Message const &message) -> bool;

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

static constexpr usize k_boot_info_alignment { alignof(BootInfoTable) };

static auto write_boot_blob(Memory::MemoryObject *object, usize offset,
    void const *src, usize size) -> bool;

static auto write_boot_struct(
    Memory::MemoryObject *object, usize offset, auto const &value) -> bool;

struct BootBlobWriter {
	Memory::MemoryObject *object {};
	usize offset {};

	auto align(usize alignment = k_boot_info_alignment) -> bool
	{
		auto const aligned { CL::Memory::align_up(offset, alignment) };
		if (aligned > object->size)
			return false;

		offset = aligned;
		return true;
	}

	template<typename T> auto write(T const &value) -> bool
	{
		return write_boot_blob(object, offset, &value, sizeof(T))
		    ? (offset += sizeof(T), true)
		    : false;
	}

	auto write_bytes(void const *src, usize size) -> bool
	{
		return write_boot_blob(object, offset, src, size)
		    ? (offset += size, true)
		    : false;
	}

	auto patch(usize at, auto const &value) -> bool
	{
		return write_boot_struct(object, at, value);
	}
};

static auto write_boot_blob(Memory::MemoryObject *object, usize offset,
    void const *src, usize size) -> bool
{
	if (!object || (!src && size != 0))
		return false;
	if (offset > object->size || size > object->size - offset)
		return false;

	auto const *bytes { static_cast<u8 const *>(src) };
	usize copied {};
	while (copied < size) {
		auto const position { offset + copied };
		auto const page_index { position / Arch::k_page_size };
		auto const page_offset { position % Arch::k_page_size };
		auto const chunk {
			(Arch::k_page_size - page_offset) < (size - copied)
			    ? (Arch::k_page_size - page_offset)
			    : (size - copied),
		};

		auto const phys_opt { object->pages.get(page_index) };
		if (!phys_opt)
			return false;

		auto *page { Arch::Paging::phys_to_virt(*phys_opt) };
		memcpy(static_cast<u8 *>(page) + page_offset, bytes + copied, chunk);
		copied += chunk;
	}

	return true;
}

static auto write_boot_struct(
    Memory::MemoryObject *object, usize offset, auto const &value) -> bool
{
	return write_boot_blob(object, offset, &value, sizeof(value));
}

static auto compute_boot_info_size(BootData const &boot) -> usize
{
	usize total { sizeof(BootInfoTable) };

	auto const module_count { boot.modules.size() };
	total = CL::Memory::align_up(total, k_boot_info_alignment);
	total += sizeof(BootModuleList);
	total += module_count * sizeof(BootModuleEntry);

	for (usize i {}; i < module_count; ++i) {
		auto const &module { boot.modules[i] };
		total = CL::Memory::align_up(total, k_boot_info_alignment);
		total += module.name.size();
	}

	total = CL::Memory::align_up(total, k_boot_info_alignment);
	if (!boot.cmdline.text.empty()) {
		auto const cmdline_len { boot.cmdline.text.size() };
		total += sizeof(BootCmdline) + cmdline_len;
	}

	auto const framebuffer_count { boot.framebuffers.size() };
	total = CL::Memory::align_up(total, k_boot_info_alignment);
	total += sizeof(BootFramebufferList);
	total += framebuffer_count * sizeof(BootFramebufferEntry);

	for (usize i {}; i < framebuffer_count; ++i) {
		auto const &framebuffer { boot.framebuffers[i] };
		total = CL::Memory::align_up(total, k_boot_info_alignment);
		total += framebuffer.edid.size();
	}

	return CL::Memory::align_up(total, k_boot_info_alignment);
}

static auto build_boot_info_object(BootData const &boot)
    -> Memory::MemoryObject *
{
	auto const total_size { compute_boot_info_size(boot) };
	if (total_size == 0)
		return nullptr;

	auto *object { Memory::MemoryObject::create_allocated(total_size) };
	if (!object)
		return nullptr;

	BootBlobWriter writer { .object = object };

	BootInfoTable table {
		.total_len = total_size,
		.modules_offset = 0,
		.modules_len = 0,
		.cmdline_offset = 0,
		.cmdline_len = 0,
		.framebuffers_offset = 0,
		.framebuffers_len = 0,
	};

	if (!writer.write(table)) {
		delete object;
		return nullptr;
	}

	if (!writer.align()) {
		delete object;
		return nullptr;
	}

	BootModuleList module_list {
		.len = 0,
		.entries_offset = 0,
	};
	auto const module_count { boot.modules.size() };
	module_list.len = module_count;
	module_list.entries_offset = writer.offset + sizeof(module_list);
	table.modules_offset = writer.offset;
	table.modules_len
	    = sizeof(module_list) + module_count * sizeof(BootModuleEntry);
	if (!writer.write(module_list)) {
		delete object;
		return nullptr;
	}

	for (usize i {}; i < module_count; ++i) {
		auto const &module { boot.modules[i] };
		BootModuleEntry entry {
			.id = i,
			.name_offset = 0,
			.name_len = 0,
			.size = module.size,
			.flags = 0,
		};
		if (!writer.write(entry)) {
			delete object;
			return nullptr;
		}
	}

	for (usize i {}; i < module_count; ++i) {
		auto const &module { boot.modules[i] };
		auto const *name { module.name.data() };
		auto const name_len { module.name.size() };
		if (!writer.align()) {
			delete object;
			return nullptr;
		}
		auto const name_offset { writer.offset };
		BootModuleEntry entry {
			.id = i,
			.name_offset = name_offset,
			.name_len = name_len,
			.size = module.size,
			.flags = 0,
		};
		auto const entry_offset {
			table.modules_offset + sizeof(BootModuleList)
			    + i * sizeof(BootModuleEntry),
		};
		if (!writer.patch(entry_offset, entry)) {
			delete object;
			return nullptr;
		}
		if (name_len != 0 && !writer.write_bytes(name, name_len)) {
			delete object;
			return nullptr;
		}
	}

	if (!boot.cmdline.text.empty()) {
		auto const cmdline { boot.cmdline.text.data() };
		auto const cmdline_len { boot.cmdline.text.size() };
		BootCmdline cmdline_blob { .len = cmdline_len };
		if (!writer.align()) {
			delete object;
			return nullptr;
		}
		table.cmdline_offset = writer.offset;
		table.cmdline_len = sizeof(BootCmdline) + cmdline_len;
		if (!writer.write(cmdline_blob)) {
			delete object;
			return nullptr;
		}
		if (cmdline_len != 0 && !writer.write_bytes(cmdline, cmdline_len)) {
			delete object;
			return nullptr;
		}
	}

	if (!writer.align()) {
		delete object;
		return nullptr;
	}

	BootFramebufferList framebuffer_list { .len = 0, .entries_offset = 0 };
	auto const framebuffer_count { boot.framebuffers.size() };
	framebuffer_list.len = framebuffer_count;
	framebuffer_list.entries_offset = writer.offset + sizeof(framebuffer_list);
	table.framebuffers_offset = writer.offset;
	table.framebuffers_len = sizeof(framebuffer_list)
	    + framebuffer_count * sizeof(BootFramebufferEntry);
	if (!writer.write(framebuffer_list)) {
		delete object;
		return nullptr;
	}

	for (usize i {}; i < framebuffer_count; ++i) {
		auto const &framebuffer { boot.framebuffers[i] };
		BootFramebufferEntry entry {
			.id = i,
			.width = framebuffer.width,
			.height = framebuffer.height,
			.pitch = framebuffer.pitch,
			.bpp = framebuffer.bpp,
			.format = framebuffer.format,
			.size = framebuffer.pitch * framebuffer.height,
			.edid_len = framebuffer.edid.size(),
			.edid_offset = 0,
			.flags = 0,
		};
		if (entry.edid_len != 0) {
			if (!writer.align()) {
				delete object;
				return nullptr;
			}
			entry.edid_offset = writer.offset;
			if (!writer.write_bytes(framebuffer.edid.data(),
			        static_cast<usize>(entry.edid_len))) {
				delete object;
				return nullptr;
			}
		}

		auto const entry_offset {
			table.framebuffers_offset + sizeof(BootFramebufferList)
			    + i * sizeof(BootFramebufferEntry),
		};
		if (!writer.patch(entry_offset, entry)) {
			delete object;
			return nullptr;
		}
	}

	table.total_len = writer.offset;
	if (!writer.patch(0, table)) {
		delete object;
		return nullptr;
	}

	Memory::retain_memory_object(object);
	return object;
}

static auto boot_data() -> BootData const *&
{
	static BootData const *data {};
	return data;
}

static auto boot_info_object() -> Memory::MemoryObject *&
{
	static Memory::MemoryObject *object {};
	return object;
}

static auto ensure_boot_info_object() -> Memory::MemoryObject *
{
	auto &object { boot_info_object() };
	if (!object) {
		auto const *boot { boot_data() };
		if (!boot)
			return nullptr;
		object = build_boot_info_object(*boot);
	}
	return object;
}

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

static auto send_kernel_ipc_reply(u64 endpoint, IPC::Message const &message)
    -> bool
{
	auto *process { Arch::Scheduler::the().process_by_endpoint(endpoint) };
	if (!process)
		return false;

	return process->ipc_message_queue.try_push(message);
}

auto kernel_handle_ipc_message(IPC::Message const &message) -> bool
{
	if (message.reply_to != 0)
		return false;

	auto boot_info_sender { [](u64 endpoint) -> Arch::Process * {
		auto *process { Arch::Scheduler::the().process_by_endpoint(endpoint) };
		if (!process
		    || !process->capabilities.contains(ProcessCapability::BootInfo))
			return nullptr;
		return process;
	} };

	auto send_handle_reply { [](IPC::Message const &request,
		                         Memory::MemoryObject *object, u64 status,
		                         char const *error_message) -> void {
		auto *sender_process {
			Arch::Scheduler::the().process_by_endpoint(request.sender),
		};
		if (!sender_process)
			return;

		auto reply_builder { IPC::make_reply(request,
			IPC::type_of<KernelMessageType>(request), object ? status : 1) };

		auto handle { Katline::Handle {} };
		if (object) {
			handle = Arch::HandleManager::the().open(
			    sender_process, Arch::HandleKind::MemoryObject, object);
			if (handle.is_invalid()) {
				Debug::print_formatted("[IPC] %s endpoint %d\n", error_message,
				    static_cast<int>(request.sender));
				return;
			}
			reply_builder.handle<0>(handle);
		}
		auto const reply { reply_builder.build() };

		if (!send_kernel_ipc_reply(request.sender, reply)) {
			if (object)
				Arch::HandleManager::the().close(sender_process, handle);
			Debug::print_formatted(
			    "[IPC] failed to enqueue %s reply for endpoint %d\n",
			    error_message, static_cast<int>(request.sender));
		}
	} };

	auto build_boot_module_object { [](u64 id) -> Memory::MemoryObject * {
		auto const *boot { boot_data() };
		if (!boot || id >= boot->modules.size())
			return nullptr;

		auto const &module { boot->modules[id] };
		if (module.size == 0 || !module.address)
			return nullptr;

		auto *object { Memory::MemoryObject::create_allocated(
			static_cast<usize>(module.size)) };
		if (!object)
			return nullptr;

		if (!write_boot_blob(
		        object, 0, module.address, static_cast<usize>(module.size))) {
			delete object;
			return nullptr;
		}

		return object;
	} };

	auto build_boot_cmdline_object { []() -> Memory::MemoryObject * {
		auto const *boot { boot_data() };
		if (!boot || boot->cmdline.text.empty())
			return nullptr;

		auto const len { boot->cmdline.text.size() };
		auto *object { Memory::MemoryObject::create_allocated(len + 1) };
		if (!object)
			return nullptr;

		if (!write_boot_blob(object, 0, boot->cmdline.text.data(), len)) {
			delete object;
			return nullptr;
		}

		char const nul { '\0' };
		if (!write_boot_blob(object, len, &nul, 1)) {
			delete object;
			return nullptr;
		}

		return object;
	} };

	auto build_boot_framebuffer_object { [](u64 id) -> Memory::MemoryObject * {
		auto const *boot { boot_data() };
		if (!boot || id >= boot->framebuffers.size())
			return nullptr;

		auto const &framebuffer { boot->framebuffers[id] };
		if (!framebuffer.address || framebuffer.pitch == 0
		    || framebuffer.height == 0)
			return nullptr;

		return Memory::MemoryObject::create_mmio(
		    reinterpret_cast<uptr>(framebuffer.address),
		    static_cast<usize>(framebuffer.pitch * framebuffer.height));
	} };

	auto const type { IPC::type_of<KernelMessageType>(message) };
	switch (type) {
	case KernelMessageType::GetBootInfo: {
		auto *sender_process { boot_info_sender(message.sender) };
		if (!sender_process) {
			auto const reply {
				IPC::make_reply(message, KernelMessageType::GetBootInfo, 1)
				    .build(),
			};
			if (!send_kernel_ipc_reply(message.sender, reply)) {
				Debug::print_formatted("[IPC] failed to enqueue denied boot "
				                       "info reply for endpoint %d\n",
				    message.sender);
			}
			return true;
		}

		auto *object { ensure_boot_info_object() };
		send_handle_reply(
		    message, object, 0, "failed to open boot info handle");
		return true;
	}
	case KernelMessageType::GetBootModuleMemoryObjectById: {
		if (!boot_info_sender(message.sender))
			return false;
		auto *object { build_boot_module_object(message.args[0]) };
		send_handle_reply(
		    message, object, 0, "failed to open boot module handle");
		return true;
	}
	case KernelMessageType::GetCmdlineMemoryObject: {
		if (!boot_info_sender(message.sender))
			return false;
		auto *object { build_boot_cmdline_object() };
		send_handle_reply(
		    message, object, 0, "failed to open boot cmdline handle");
		return true;
	}
	case KernelMessageType::GetFramebufferMemoryObjectById: {
		if (!boot_info_sender(message.sender))
			return false;
		auto *object { build_boot_framebuffer_object(message.args[0]) };
		send_handle_reply(
		    message, object, 0, "failed to open boot framebuffer handle");
		return true;
	}
	case KernelMessageType::GetPageSize: {
		auto const reply {
			IPC::make_reply(message, KernelMessageType::GetPageSize)
			    .arg<0>(Arch::k_page_size)
			    .build(),
		};
		if (!send_kernel_ipc_reply(message.sender, reply)) {
			Debug::print_formatted(
			    "[IPC] failed to enqueue page size reply for endpoint %d\n",
			    static_cast<int>(message.sender));
		}
		return true;
	}
	default:
		return false;
	}
}

static auto pump_kernel_ipc() -> void
{
	auto *kernel_process { Arch::k_process };
	if (!kernel_process)
		return;

	while (auto message { kernel_process->ipc_message_queue.try_pop() })
		CL::ignore_unused(kernel_handle_ipc_message(*message));
}

auto katline_main(StartupInfo &info, BootData const &boot) -> void
{
	boot_data() = &boot;
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
	if (!ensure_boot_info_object())
		kpanic("[IPC] failed to build boot info blob\n");
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
		pump_kernel_ipc();
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
