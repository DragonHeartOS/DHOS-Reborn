#include <new>
#include <string.h>

import CommonLib;
import KatlineAPI;

namespace {

static constexpr u64 k_boot_cookie_base { 0x42554f5453544152ull };
static constexpr u64 k_bootstrap_endpoint { 1 };
static constexpr u64 k_loader_boot_endpoint { 2 };
static constexpr u64 k_loader_stack_size { 0x4000 };
static constexpr u64 k_loader_stack_top { 0x00007fffffffe000ull };
static constexpr u64 k_bootstrap_endpoint_mailbox { 0x00000000fff00000ull };
static constexpr u64 k_boot_archive_base { 0x0000000100000000ull };
static constexpr char k_boot_archive_name[] { "boot.tar" };
static constexpr char k_loader_name[] { "Loader.exe" };
static constexpr char k_order_file_name[] { "load-order.txt" };

enum class BootLoaderMessageType : u64 {
	Ready = 0x100,
	LoadExecutable = 0x101,
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

struct TarEntryView {
	bool found {};
	u8 const *data {};
	u64 size {};
};

struct TextLineView {
	char const *data {};
	u64 size {};
	u64 next_offset {};
};

static auto next_cookie() -> u64
{
	static u64 cookie { k_boot_cookie_base };
	return ++cookie;
}

static auto debug_point(u64 tag, u64 value) -> void
{
	Katline::Syscalls::sys_debug_u64(tag, value);
	(void)value;
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

static auto raw_thread_create(Katline::Handle process, u64 entry, u64 stack)
    -> Katline::Handle
{
	auto const raw {
		Katline::Syscalls::syscall_raw(
		    static_cast<u64>(Katline::Syscalls::SyscallNumber::ThreadCreate),
		    process.id, entry, stack, 0, 0),
	};
	debug_point(0xB101, raw);
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
	debug_point(0xB102, raw);
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

static auto find_boot_module(Katline::BootInfoTable const *boot_info,
    u8 const *boot_blob, char const *name, u64 name_len) -> BootModuleView
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
		auto const *entry_name {
			reinterpret_cast<char const *>(boot_blob + entry.name_offset),
		};
		auto const *match_name { entry_name };
		u64 match_len { entry.name_len };
		for (u64 j {}; j < entry.name_len; ++j) {
			if (entry_name[j] == '/') {
				match_name = entry_name + j + 1;
				match_len = entry.name_len - j - 1;
			}
		}
		if ((entry.name_len == name_len
		        && memcmp(
		               entry_name, name, static_cast<unsigned long>(name_len))
		            == 0)
		    || (match_len == name_len
		        && memcmp(
		               match_name, name, static_cast<unsigned long>(name_len))
		            == 0)) {
			return BootModuleView {
				.found = true,
				.id = entry.id,
				.size = entry.size,
			};
		}
	}

	return {};
}

static auto parse_tar_size(u8 const *field) -> u64
{
	u64 value {};
	for (u64 i {}; i < 12; ++i) {
		auto const byte { field[i] };
		if (byte == 0 || byte == ' ')
			break;
		value = value * 8 + static_cast<u64>(byte - '0');
	}
	return value;
}

static auto tar_field_size(u8 const *field, u64 max_size) -> u64
{
	u64 size {};
	while (size < max_size && field[size] != 0)
		++size;
	return size;
}

static auto find_tar_file(u8 const *archive, u64 archive_size, char const *name,
    u64 name_size) -> TarEntryView
{
	static constexpr u64 k_tar_block_size { 512 };
	static constexpr u64 k_tar_header_size_offset { 124 };

	if (!archive || archive_size < k_tar_block_size)
		return {};

	for (u64 offset {}; offset <= archive_size - k_tar_block_size;) {
		auto const *header { archive + offset };
		if (header[0] == 0)
			return {};

		auto const name_size_in_header { tar_field_size(header, 100) };
		auto const file_size {
			parse_tar_size(header + k_tar_header_size_offset),
		};
		auto const data_offset { offset + k_tar_block_size };
		if (file_size > archive_size - data_offset)
			return {};
		auto const padded_size {
			(file_size + k_tar_block_size - 1) & ~(k_tar_block_size - 1),
		};
		if (padded_size > archive_size - data_offset)
			return {};

		if (name_size_in_header == name_size
		    && memcmp(header, name, static_cast<unsigned long>(name_size))
		        == 0) {
			return TarEntryView {
				.found = true,
				.data = archive + data_offset,
				.size = file_size,
			};
		}

		offset = data_offset + padded_size;
	}

	return {};
}

static auto next_text_line(
    char const *text, u64 text_size, u64 offset, TextLineView *line_out) -> bool
{
	if (!text || !line_out || offset >= text_size)
		return false;
	auto end { offset };
	while (end < text_size && text[end] != '\n')
		++end;
	auto line_size { end - offset };
	if (line_size != 0 && text[offset + line_size - 1] == '\r')
		--line_size;
	if (line_size == 0)
		return false;
	*line_out = TextLineView {
		.data = text + offset,
		.size = line_size,
		.next_offset = end < text_size ? end + 1 : end,
	};
	return true;
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

		auto const segment_base { segment.p_vaddr };
		auto const segment_end { segment.p_vaddr + segment.p_memsz };
		if (page_base >= segment_end || page_end <= segment_base)
			continue;

		overlaps = true;
		flags |= elf_segment_flags(segment);
	}

	if (flags_out)
		*flags_out = flags;
	return overlaps;
}

static auto start_elf_process(Katline::Handle process, void const *image,
    u64 image_size, u64 page_size) -> bool
{
	if (!image || image_size < sizeof(ELF::Header) || page_size == 0) {
		debug_point(0xB100, 1);
		return false;
	}

	auto const *header { reinterpret_cast<ELF::Header const *>(image) };
	if (header->e_ident[0] != 0x7f || header->e_ident[1] != 'E'
	    || header->e_ident[2] != 'L' || header->e_ident[3] != 'F') {
		debug_point(0xB100, 2);
		return false;
	}
	if (header->e_phentsize != sizeof(ELF::ProgramHeader)) {
		debug_point(0xB100, 3);
		return false;
	}
	if (header->e_type != ELF::ET_EXEC && header->e_type != ELF::ET_DYN) {
		debug_point(0xB100, 4);
		return false;
	}
	if (header->e_phoff
	        + static_cast<u64>(header->e_phnum) * header->e_phentsize
	    > image_size) {
		debug_point(0xB100, 5);
		return false;
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
	if (!found_load || max_vaddr <= min_vaddr) {
		debug_point(0xB100, 14);
		return false;
	}

	auto const load_base { CL::Memory::align_down(min_vaddr, page_size) };
	auto const total_size { CL::Memory::align_up(
		max_vaddr - load_base, page_size) };
	auto const image_handle_res {
		Katline::Syscalls::sys_memory_create(total_size),
	};
	if (image_handle_res.is_err()) {
		debug_point(0xB100, 15);
		return false;
	}
	auto const image_handle { image_handle_res.get_ok_unsafe() };

	void *image_local_map {};
	auto *image_local {
		map_memory_object<u8>(image_handle, 0, total_size,
		    Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		    &image_local_map),
	};
	if (!image_local) {
		debug_point(0xB100, 16);
		Katline::Syscalls::sys_handle_close(image_handle);
		return false;
	}

	for (u16 i {}; i < header->e_phnum; ++i) {
		auto const &segment { program_headers[i] };
		if (segment.p_type != ELF::PT_LOAD)
			continue;
		if (segment.p_filesz > segment.p_memsz
		    || segment.p_offset + segment.p_filesz > image_size) {
			debug_point(0xB100, 6);
			return false;
		}

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
		auto const remote_map_raw { raw_process_memory_map(&options) };
		debug_point(0xB103, static_cast<u64>(remote_map_raw));
		if (remote_map_raw < 0) {
			debug_point(0xB100, 9);
			Katline::Syscalls::sys_memory_unmap(image_local_map, total_size);
			Katline::Syscalls::sys_handle_close(image_handle);
			return false;
		}
	}

	Katline::Syscalls::sys_memory_unmap(image_local_map, total_size);
	Katline::Syscalls::sys_handle_close(image_handle);

	auto const stack_handle_res {
		Katline::Syscalls::sys_memory_create(k_loader_stack_size),
	};
	if (stack_handle_res.is_err()) {
		debug_point(0xB100, 10);
		return false;
	}
	auto const stack_handle { stack_handle_res.get_ok_unsafe() };
	Katline::ProcessMemoryMapOptions stack_options {
		.process = process,
		.memory_object = stack_handle,
		.offset = 0,
		.size = k_loader_stack_size,
		.flags = Katline::MemoryMapFlags {
			Katline::MemoryMapFlag::Writable,
			Katline::MemoryMapFlag::NoExecute,
		},
		.addr = reinterpret_cast<void *>(k_loader_stack_top - k_loader_stack_size),
	};
	auto const stack_map_raw { raw_process_memory_map(&stack_options) };
	debug_point(0xB104, static_cast<u64>(stack_map_raw));
	if (stack_map_raw < 0) {
		debug_point(0xB100, 11);
		Katline::Syscalls::sys_handle_close(stack_handle);
		return false;
	}
	Katline::Syscalls::sys_handle_close(stack_handle);

	auto const thread { raw_thread_create(
		process, header->e_entry, k_loader_stack_top) };
	if (thread.is_invalid()) {
		debug_point(0xB100, 12);
		return false;
	}

	if (!raw_thread_start(thread)) {
		debug_point(0xB100, 13);
		Katline::Syscalls::sys_handle_close(thread);
		return false;
	}

	return true;
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

extern "C" [[noreturn]] auto _start() -> void
{
	debug_point(0xB000, 0x1);
	auto const page_size_reply {
		send_kernel_message(Katline::KernelMessageType::GetPageSize),
	};
	auto const page_size { page_size_reply.args[0] };
	if (page_size == 0)
		fatal(2);
	debug_point(0xB000, 0x2);

	auto const boot_reply {
		send_kernel_message(Katline::KernelMessageType::GetBootInfo),
	};
	if (boot_reply.status != 0 || boot_reply.handles[0].is_invalid())
		fatal(3);
	debug_point(0xB000, 0x3);

	auto const boot_handle { boot_reply.handles[0] };
	void *boot_info_map {};
	auto const *boot_info {
		map_memory_object<Katline::BootInfoTable const>(boot_handle, 0,
		    sizeof(Katline::BootInfoTable),
		    Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		    &boot_info_map),
	};
	if (!boot_info)
		fatal(4);

	void *boot_blob_map {};
	auto const *boot_blob {
		map_memory_object<u8 const>(boot_handle, 0, boot_info->total_len,
		    Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		    &boot_blob_map),
	};
	if (!boot_blob)
		fatal(5);

	auto const boot_archive_module {
		find_boot_module(boot_info, boot_blob, k_boot_archive_name,
		    sizeof(k_boot_archive_name) - 1),
	};
	if (!boot_archive_module.found)
		fatal(6);
	debug_point(0xB001, boot_archive_module.id);

	auto const boot_archive_reply {
		send_kernel_message(
		    Katline::KernelMessageType::GetBootModuleMemoryObjectById,
		    boot_archive_module.id),
	};
	if (boot_archive_reply.status != 0
	    || boot_archive_reply.handles[0].is_invalid())
		fatal(9);

	void *boot_archive_map {};
	auto const *boot_archive {
		map_memory_object<u8 const>(boot_archive_reply.handles[0], 0,
		    boot_archive_module.size,
		    Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		    &boot_archive_map),
	};
	if (!boot_archive)
		fatal(10);

	auto const order_file {
		find_tar_file(boot_archive, boot_archive_module.size, k_order_file_name,
		    sizeof(k_order_file_name) - 1),
	};
	if (!order_file.found)
		fatal(11);
	TextLineView loader_order_entry {};
	if (!next_text_line(reinterpret_cast<char const *>(order_file.data),
	        order_file.size, 0, &loader_order_entry)
	    || loader_order_entry.size != sizeof(k_loader_name) - 1
	    || memcmp(loader_order_entry.data, k_loader_name,
	           static_cast<unsigned long>(loader_order_entry.size))
	        != 0)
		fatal(11);

	auto const loader_file {
		find_tar_file(boot_archive, boot_archive_module.size, k_loader_name,
		    sizeof(k_loader_name) - 1),
	};
	if (!loader_file.found)
		fatal(11);

	auto const boot_blob_size { boot_info->total_len };
	Katline::Syscalls::sys_memory_unmap(
	    boot_info_map, sizeof(Katline::BootInfoTable));
	Katline::Syscalls::sys_memory_unmap(boot_blob_map, boot_blob_size);

	auto const loader_process_res { Katline::Syscalls::sys_process_create() };
	if (loader_process_res.is_err())
		fatal(7);
	auto const loader_process { loader_process_res.get_ok_unsafe() };
	debug_point(0xB002, loader_process.id);

	// Needs all capabilities since it may load device drivers and other very
	// important processes.
	auto const loader_caps { Katline::ProcessCapabilityFlags::all() };
	if (auto const grant_res {
	        Katline::Syscalls::sys_process_grant_capabilities(
	            loader_process, loader_caps),
	    };
	    grant_res.is_err()) {
		fatal(8);
	}
	debug_point(0xB003, 0);

	auto const mailbox_handle_res {
		Katline::Syscalls::sys_memory_create(sizeof(u64)),
	};
	if (mailbox_handle_res.is_err())
		fatal(8);
	auto const mailbox_handle { mailbox_handle_res.get_ok_unsafe() };

	void *mailbox_local_map {};
	auto *mailbox_local {
		map_memory_object<u64>(mailbox_handle, 0, sizeof(u64),
		    Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		    &mailbox_local_map),
	};
	if (!mailbox_local)
		fatal(8);
	*mailbox_local = k_bootstrap_endpoint;

	Katline::ProcessMemoryMapOptions mailbox_options {
		.process = loader_process,
		.memory_object = mailbox_handle,
		.offset = 0,
		.size = sizeof(u64),
		.flags = Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		.addr = reinterpret_cast<void *>(k_bootstrap_endpoint_mailbox),
	};
	if (raw_process_memory_map(&mailbox_options) < 0)
		fatal(8);
	Katline::Syscalls::sys_memory_unmap(mailbox_local_map, sizeof(u64));
	Katline::Syscalls::sys_handle_close(mailbox_handle);

	Katline::ProcessMemoryMapOptions archive_options {
		.process = loader_process,
		.memory_object = boot_archive_reply.handles[0],
		.offset = 0,
		.size = boot_archive_module.size,
		.flags = Katline::MemoryMapFlags { Katline::MemoryMapFlag::NoExecute },
		.addr = reinterpret_cast<void *>(k_boot_archive_base),
	};
	if (raw_process_memory_map(&archive_options) < 0)
		fatal(10);

	if (!start_elf_process(
	        loader_process, loader_file.data, loader_file.size, page_size))
		fatal(11);

	for (;;) {
		auto const message { receive_message() };
		if (message.sender != k_loader_boot_endpoint
		    || static_cast<BootLoaderMessageType>(message.type)
		        != BootLoaderMessageType::Ready) {
			continue;
		}
		break;
	}

	auto const service_caps {
		Katline::ProcessCapabilityFlags {
		    Katline::ProcessCapability::ManageCaps,
		    Katline::ProcessCapability::BootInfo,
		    Katline::ProcessCapability::MapMMIO,
		    Katline::ProcessCapability::ManageProcesses,
		},
	};
	auto const *order_text {
		reinterpret_cast<char const *>(order_file.data),
	};
	auto order_offset { loader_order_entry.next_offset };
	for (;;) {
		TextLineView entry_name {};
		if (!next_text_line(
		        order_text, order_file.size, order_offset, &entry_name))
			break;
		order_offset = entry_name.next_offset;
		auto const executable {
			find_tar_file(boot_archive, boot_archive_module.size,
			    entry_name.data, entry_name.size),
		};
		if (!executable.found)
			fatal(12);

		auto const executable_address {
			k_boot_archive_base
			    + static_cast<u64>(executable.data - boot_archive),
		};
		auto const request {
			Katline::IPC::make_request(BootLoaderMessageType::LoadExecutable)
			    .cookie(next_cookie())
			    .arg<0>(executable_address)
			    .arg<1>(executable.size)
			    .arg<2>(service_caps.raw())
			    .build(),
		};
		if (!send_message(k_loader_boot_endpoint, request))
			fatal(13);

		for (;;) {
			auto const reply { receive_message() };
			if (reply.sender != k_loader_boot_endpoint
			    || reply.cookie != request.cookie)
				continue;
			if (reply.status != 0)
				fatal(14);
			break;
		}
	}

	Katline::Syscalls::sys_memory_unmap(
	    boot_archive_map, boot_archive_module.size);
	Katline::Syscalls::sys_handle_close(boot_archive_reply.handles[0]);

	Katline::Syscalls::sys_handle_close(loader_process);
	Katline::Syscalls::sys_handle_close(boot_handle);

	Katline::Syscalls::sys_exit(0);
	debug_point(0xB00A, 0xDEADBEEF);
	__builtin_trap();
}
