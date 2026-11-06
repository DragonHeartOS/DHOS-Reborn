#include <string.h>

import CommonLib;
import KatlineAPI;

namespace {

static constexpr u64 k_boot_cookie_base { 0x42554f5453544152ull };
static constexpr u64 k_bootstrap_endpoint_mailbox { 0x00000000fff00000ull };
static constexpr u64 k_user_stack_size { 0x4000 };
static constexpr u64 k_user_stack_top { 0x00007fffffffe000ull };

enum class BootLoaderMessageType : u64 {
	Ready = 0x100,
	LoadBootModuleById = 0x101,
};

namespace ELF {

constexpr u16 EI_NIDENT { 16 };
constexpr u16 ET_EXEC { 2 };
constexpr u16 ET_DYN { 3 };
constexpr u32 PT_LOAD { 1 };
constexpr u32 PF_X { 1u << 0 };
constexpr u32 PF_W { 1u << 1 };

struct Header {
	unsigned char e_ident[EI_NIDENT];
	u16 e_type;
	u16 e_machine;
	u32 e_version;
	u64 e_entry;
	u64 e_phoff;
	u64 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shstrndx;
};

struct ProgramHeader {
	u32 p_type;
	u32 p_flags;
	u64 p_offset;
	u64 p_vaddr;
	u64 p_paddr;
	u64 p_filesz;
	u64 p_memsz;
	u64 p_align;
};

} // namespace ELF

struct BootModuleView {
	bool found {};
	u64 id {};
	u64 size {};
};

static auto next_cookie() -> u64
{
	static u64 cookie { k_boot_cookie_base };
	return ++cookie;
}

[[noreturn]] static auto fatal(u64 code) -> void
{
	Katline::Syscalls::sys_exit(static_cast<i32>(code));
	__builtin_trap();
}

static auto send_message(u64 endpoint, Katline::IPC::Message const &message)
    -> bool
{
	for (;;) {
		auto const res { Katline::Syscalls::sys_ipc_send(
			endpoint, &message, 0) };
		if (res.is_ok())
			return true;
		if (!res.unwrap_err()
		        .template is<Katline::Syscalls::ErrorsV::QueueFull>())
			return false;
		Katline::Syscalls::sys_yield();
	}
}

static auto send_kernel_message(Katline::KernelMessageType type, u64 arg0 = 0)
    -> Katline::IPC::Message
{
	auto builder { Katline::IPC::make_request(type).cookie(next_cookie()) };
	if (type == Katline::KernelMessageType::GetBootModuleMemoryObjectById)
		builder.arg<0>(arg0);
	auto const request { builder.build() };
	if (!send_message(0, request))
		fatal(1);

	for (;;) {
		Katline::IPC::Message reply {};
		auto const res { Katline::Syscalls::sys_ipc_receive(&reply, 0) };
		if (res.is_err()) {
			Katline::Syscalls::sys_yield();
			continue;
		}
		if (reply.cookie == request.cookie)
			return reply;
	}
}

static auto bootstrap_endpoint() -> u64
{
	auto const *mailbox {
		reinterpret_cast<u64 const *>(k_bootstrap_endpoint_mailbox),
	};
	return mailbox ? *mailbox : 0;
}

static auto raw_thread_create(Katline::Handle process, u64 entry, u64 stack)
    -> Katline::Handle
{
	auto const raw {
		Katline::Syscalls::syscall_raw(
		    static_cast<u64>(Katline::Syscalls::SyscallNumber::ThreadCreate),
		    process.id, entry, stack, 0, 0),
	};
	Katline::Syscalls::sys_debug_u64(0x1001, raw);
	if (static_cast<i64>(raw) < 0)
		return {};
	return Katline::Handle { raw };
}

static auto raw_thread_start(Katline::Handle thread) -> bool
{
	auto const raw {
		Katline::Syscalls::syscall_raw(
		    static_cast<u64>(Katline::Syscalls::SyscallNumber::ThreadStart),
		    thread.id, 0, 0, 0, 0),
	};
	return static_cast<i64>(raw) >= 0;
}

static auto raw_process_memory_map(Katline::ProcessMemoryMapOptions *options)
    -> i64
{
	return static_cast<i64>(Katline::Syscalls::syscall_raw(
	    static_cast<u64>(Katline::Syscalls::SyscallNumber::ProcessMemoryMap),
	    reinterpret_cast<u64>(options), 0, 0, 0, 0));
}

template<typename T>
static auto map_memory_object(Katline::Handle handle, u64 offset, u64 size,
    Katline::MemoryMapFlags flags, void **mapped_out) -> T *
{
	auto const res {
		Katline::Syscalls::sys_memory_map(
		    handle, offset, size, flags, mapped_out),
	};
	if (res.is_err())
		return nullptr;
	return reinterpret_cast<T *>(*res);
}

static auto find_boot_module_by_id(Katline::BootInfoTable const *boot_info,
    u8 const *boot_blob, u64 module_id) -> BootModuleView
{
	auto const *module_list {
		reinterpret_cast<Katline::BootModuleList const *>(
		    boot_blob + boot_info->modules_offset),
	};
	auto const *entries {
		reinterpret_cast<Katline::BootModuleEntry const *>(
		    boot_blob + module_list->entries_offset),
	};

	for (u64 i {}; i < module_list->len; ++i) {
		auto const &entry { entries[i] };
		if (entry.id != module_id)
			continue;
		return BootModuleView {
			.found = true,
			.id = entry.id,
			.size = entry.size,
		};
	}

	return {};
}

static auto elf_segment_flags(ELF::ProgramHeader const &header)
    -> Katline::MemoryMapFlags
{
	Katline::MemoryMapFlags flags {};
	if ((header.p_flags & ELF::PF_W) != 0)
		flags |= Katline::MemoryMapFlag::Writable;
	if ((header.p_flags & ELF::PF_X) == 0)
		flags |= Katline::MemoryMapFlag::NoExecute;
	return flags;
}

static auto page_flags_for_loaded_image(
    ELF::ProgramHeader const *program_headers, u16 phnum, u64 page_base,
    u64 page_size, Katline::MemoryMapFlags *flags_out) -> bool
{
	Katline::MemoryMapFlags flags {};
	auto const page_end { page_base + page_size };
	bool overlaps {};

	for (u16 i {}; i < phnum; ++i) {
		auto const &segment { program_headers[i] };
		if (segment.p_type != ELF::PT_LOAD || segment.p_memsz == 0)
			continue;
		auto const segment_end { segment.p_vaddr + segment.p_memsz };
		if (page_base >= segment_end || page_end <= segment.p_vaddr)
			continue;
		overlaps = true;
		flags |= elf_segment_flags(segment);
	}

	if (flags_out)
		*flags_out = flags;
	return overlaps;
}

static auto load_elf_into_new_process(void const *image, u64 image_size,
    u64 page_size, Katline::ProcessCapabilityFlags capabilities) -> u64
{
	if (!image || image_size < sizeof(ELF::Header) || page_size == 0)
		return 1;

	auto const *header { reinterpret_cast<ELF::Header const *>(image) };
	if (header->e_ident[0] != 0x7f || header->e_ident[1] != 'E'
	    || header->e_ident[2] != 'L' || header->e_ident[3] != 'F')
		return 2;
	if (header->e_phentsize != sizeof(ELF::ProgramHeader))
		return 3;
	if (header->e_type != ELF::ET_EXEC && header->e_type != ELF::ET_DYN)
		return 4;
	if (header->e_phoff
	        + static_cast<u64>(header->e_phnum) * header->e_phentsize
	    > image_size)
		return 5;

	auto const process_res { Katline::Syscalls::sys_process_create() };
	if (process_res.is_err())
		return 6;
	auto const process { process_res.get_ok_unsafe() };

	if (capabilities.any()) {
		auto const grant_res {
			Katline::Syscalls::sys_process_grant_capabilities(
			    process, capabilities),
		};
		if (grant_res.is_err()) {
			Katline::Syscalls::sys_handle_close(process);
			return 7;
		}
	}

	auto const *program_headers {
		reinterpret_cast<ELF::ProgramHeader const *>(
		    static_cast<u8 const *>(image) + header->e_phoff),
	};

	bool found_load {};
	u64 min_vaddr { ~0ull };
	u64 max_vaddr {};
	for (u16 i {}; i < header->e_phnum; ++i) {
		auto const &segment { program_headers[i] };
		if (segment.p_type != ELF::PT_LOAD)
			continue;
		found_load = true;
		min_vaddr = min_vaddr < segment.p_vaddr ? min_vaddr : segment.p_vaddr;
		max_vaddr = max_vaddr > segment.p_vaddr + segment.p_memsz
		    ? max_vaddr
		    : segment.p_vaddr + segment.p_memsz;
	}
	if (!found_load || max_vaddr <= min_vaddr)
		return 16;

	auto const load_base { CL::Memory::align_down(min_vaddr, page_size) };
	auto const total_size { CL::Memory::align_up(
		max_vaddr - load_base, page_size) };
	auto const image_handle_res {
		Katline::Syscalls::sys_memory_create(total_size),
	};
	if (image_handle_res.is_err())
		return 17;
	auto const image_handle { image_handle_res.get_ok_unsafe() };

	void *image_local_map {};
	auto *image_local {
		map_memory_object<u8>(image_handle, 0, total_size,
		    Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		    &image_local_map),
	};
	if (!image_local) {
		Katline::Syscalls::sys_handle_close(image_handle);
		return 18;
	}

	for (u16 i {}; i < header->e_phnum; ++i) {
		auto const &segment { program_headers[i] };
		if (segment.p_type != ELF::PT_LOAD)
			continue;
		if (segment.p_filesz > segment.p_memsz
		    || segment.p_offset + segment.p_filesz > image_size)
			return 8;

		memcpy(image_local + (segment.p_vaddr - load_base),
		    static_cast<u8 const *>(image) + segment.p_offset,
		    static_cast<unsigned long>(segment.p_filesz));
		if (segment.p_memsz > segment.p_filesz) {
			memset(
			    image_local + (segment.p_vaddr - load_base) + segment.p_filesz,
			    0,
			    static_cast<unsigned long>(segment.p_memsz - segment.p_filesz));
		}
	}

	auto const page_count { total_size / page_size };
	for (u64 page_index {}; page_index < page_count;) {
		auto const page_base { load_base + page_index * page_size };
		Katline::MemoryMapFlags flags {};
		if (!page_flags_for_loaded_image(program_headers, header->e_phnum,
		        page_base, page_size, &flags)) {
			++page_index;
			continue;
		}

		auto const run_start { page_index };
		do {
			++page_index;
			Katline::MemoryMapFlags next_flags {};
			if (page_index >= page_count
			    || !page_flags_for_loaded_image(program_headers,
			        header->e_phnum, load_base + page_index * page_size,
			        page_size, &next_flags)
			    || next_flags != flags)
				break;
		} while (true);

		Katline::ProcessMemoryMapOptions options {
			.process = process,
			.memory_object = image_handle,
			.offset = run_start * page_size,
			.size = (page_index - run_start) * page_size,
			.flags = flags,
			.addr = reinterpret_cast<void *>(load_base + run_start * page_size),
		};
		if (raw_process_memory_map(&options) < 0) {
			Katline::Syscalls::sys_memory_unmap(image_local_map, total_size);
			Katline::Syscalls::sys_handle_close(image_handle);
			return 11;
		}
	}

	Katline::Syscalls::sys_memory_unmap(image_local_map, total_size);
	Katline::Syscalls::sys_handle_close(image_handle);

	auto const stack_handle_res {
		Katline::Syscalls::sys_memory_create(k_user_stack_size),
	};
	if (stack_handle_res.is_err())
		return 12;
	auto const stack_handle { stack_handle_res.get_ok_unsafe() };
	Katline::ProcessMemoryMapOptions stack_options {
		.process = process,
		.memory_object = stack_handle,
		.offset = 0,
		.size = k_user_stack_size,
		.flags = Katline::MemoryMapFlags {
			Katline::MemoryMapFlag::Writable,
			Katline::MemoryMapFlag::NoExecute,
		},
		.addr = reinterpret_cast<void *>(k_user_stack_top - k_user_stack_size),
	};
	if (raw_process_memory_map(&stack_options) < 0) {
		Katline::Syscalls::sys_handle_close(stack_handle);
		Katline::Syscalls::sys_handle_close(process);
		return 13;
	}
	Katline::Syscalls::sys_handle_close(stack_handle);

	auto const thread { raw_thread_create(
		process, header->e_entry, k_user_stack_top) };
	if (thread.is_invalid()) {
		Katline::Syscalls::sys_handle_close(process);
		return 14;
	}

	if (!raw_thread_start(thread)) {
		Katline::Syscalls::sys_handle_close(thread);
		Katline::Syscalls::sys_handle_close(process);
		return 15;
	}

	Katline::Syscalls::sys_handle_close(process);
	return 0;
}

static auto receive_message() -> Katline::IPC::Message
{
	for (;;) {
		Katline::IPC::Message message {};
		auto const res { Katline::Syscalls::sys_ipc_receive(&message, 0) };
		if (res.is_ok())
			return message;
	}
}

} // namespace

extern "C" auto dhos_main(CL::Span<CL::StringView>,
    CL::HashMap<CL::StringView, CL::StringView>) -> int
{
	Katline::Syscalls::sys_debug_u64(0x1002, 1);
	auto const page_size_reply {
		send_kernel_message(Katline::KernelMessageType::GetPageSize),
	};
	auto const page_size { page_size_reply.args[0] };
	if (page_size == 0)
		fatal(2);
	Katline::Syscalls::sys_debug_u64(0x1002, 2);

	auto const ready_message {
		Katline::IPC::make_request(BootLoaderMessageType::Ready)
		    .cookie(next_cookie())
		    .build(),
	};
	auto const bootstrap_ep { bootstrap_endpoint() };
	if (bootstrap_ep == 0 || !send_message(bootstrap_ep, ready_message))
		fatal(6);
	Katline::Syscalls::sys_debug_u64(0x1002, 3);

	void *boot_info_map {};
	void *boot_blob_map {};
	Katline::BootInfoTable const *boot_info {};
	u8 const *boot_blob {};
	Katline::Handle boot_handle {};
	auto ensure_boot_data_loaded = [&]() -> bool {
		if (boot_info && boot_blob)
			return true;

		auto const boot_reply {
			send_kernel_message(Katline::KernelMessageType::GetBootInfo),
		};
		if (boot_reply.status != 0 || boot_reply.handles[0].is_invalid())
			return false;

		boot_handle = boot_reply.handles[0];
		boot_info = map_memory_object<Katline::BootInfoTable const>(boot_handle,
		    0, sizeof(Katline::BootInfoTable),
		    Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		    &boot_info_map);
		if (!boot_info)
			return false;

		boot_blob
		    = map_memory_object<u8 const>(boot_handle, 0, boot_info->total_len,
		        Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		        &boot_blob_map);
		return boot_blob != nullptr;
	};

	Katline::Syscalls::sys_debug_u64(0x1002, 4);
	for (;;) {
		auto const request { receive_message() };
		if (static_cast<BootLoaderMessageType>(request.type)
		    != BootLoaderMessageType::LoadBootModuleById)
			continue;

		Katline::Syscalls::sys_debug_u64(0x1002, 0x4000);

		if (!ensure_boot_data_loaded()) {
			auto const reply {
				Katline::IPC::make_reply(
				    request, BootLoaderMessageType::LoadBootModuleById, 2)
				    .build(),
			};
			send_message(request.sender, reply);
			continue;
		}
		Katline::Syscalls::sys_debug_u64(0x1002, 0x4001);

		auto const module {
			find_boot_module_by_id(boot_info, boot_blob, request.args[0]),
		};
		u64 status {};
		if (!module.found) {
			status = 1;
		} else {
			auto const module_reply {
				send_kernel_message(
				    Katline::KernelMessageType::GetBootModuleMemoryObjectById,
				    module.id),
			};
			if (module_reply.status != 0
			    || module_reply.handles[0].is_invalid()) {
				status = 2;
			} else {
				void *module_map {};
				auto const *module_blob {
					map_memory_object<u8 const>(module_reply.handles[0], 0,
					    module.size,
					    Katline::MemoryMapFlags {
					        Katline::MemoryMapFlag::Writable,
					    },
					    &module_map),
				};
				if (!module_blob) {
					status = 3;
				} else {
					auto const child_caps {
						Katline::ProcessCapabilityFlags(request.args[1]),
					};
					status = load_elf_into_new_process(
					    module_blob, module.size, page_size, child_caps);
					Katline::Syscalls::sys_memory_unmap(
					    module_map, module.size);
				}
				Katline::Syscalls::sys_handle_close(module_reply.handles[0]);
			}
		}

		auto const reply {
			Katline::IPC::make_reply(
			    request, BootLoaderMessageType::LoadBootModuleById, status)
			    .build(),
		};
		send_message(request.sender, reply);
		Katline::Syscalls::sys_debug_u64(0x1002, 0x4002);
	}

	Katline::Syscalls::sys_debug_u64(0x1002, 5);
}
