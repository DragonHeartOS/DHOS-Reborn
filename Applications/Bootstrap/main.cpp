import CommonLib;
import KatlineAPI;

namespace {
static constexpr u64 k_boot_cookie_base { 0x42554f5453544152ull };

static auto next_cookie() -> u64
{
	static u64 cookie { k_boot_cookie_base };
	return ++cookie;
}

static auto debug_point(u64 tag, u64 value) -> void
{
	Katline::Syscalls::sys_debug_u64(tag, value);
}

[[noreturn]] static auto fatal(u64 code) -> void
{
	Katline::Syscalls::sys_exit(static_cast<i32>(code));
	__builtin_trap();
}

static auto send_kernel_message(Katline::KernelMessageType type)
    -> Katline::IPC::Message
{
	auto const request {
		Katline::IPC::make_request(type).cookie(next_cookie()).build(),
	};
	auto const raw_type { request.type };

	for (;;) {
		debug_point(0x1000 | raw_type, 0x1);

		auto const res { Katline::Syscalls::sys_ipc_send(0, &request, 0) };
		if (res.is_ok())
			break;

		Katline::Syscalls::sys_yield();
	}

	debug_point(0x1000 | raw_type, 0x3);

	for (;;) {
		Katline::IPC::Message reply {};
		auto const res { Katline::Syscalls::sys_ipc_receive(&reply, 0) };

		if (res.is_err()) {
			debug_point(0x2000 | raw_type, 0x2);
			Katline::Syscalls::sys_yield();
			continue;
		}

		if (reply.cookie == request.cookie)
			return reply;
	}
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

} // namespace

extern "C" [[noreturn]] auto _start() -> void
{
	debug_point(0, 0x1111);

	auto const page_size_reply {
		send_kernel_message(Katline::KernelMessageType::GetPageSize),
	};

	auto const page_size { page_size_reply.args[0] };
	if (page_size == 0)
		fatal(0);

	debug_point(1, page_size);

	auto const boot_reply {
		send_kernel_message(Katline::KernelMessageType::GetBootInfo),
	};

	if (boot_reply.status != 0 || boot_reply.handles[0].is_invalid())
		fatal(1);

	auto const boot_handle { boot_reply.handles[0] };
	debug_point(2, boot_handle.id);

	void *boot_header_map {};

	auto const *boot_header {
		map_memory_object<Katline::BootInfoTable const>(boot_handle, 0,
		    sizeof(Katline::BootInfoTable),
		    Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		    &boot_header_map),
	};
	if (!boot_header)
		fatal(2);

	auto const boot_total_len { boot_header->total_len };
	void *boot_blob_map {};
	auto const *boot_blob {
		map_memory_object<u8 const>(boot_handle, 0, boot_total_len,
		    Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		    &boot_blob_map),
	};
	if (!boot_blob)
		fatal(3);

	auto const *boot_info {
		reinterpret_cast<Katline::BootInfoTable const *>(boot_blob),
	};

	debug_point(3, boot_total_len);
	debug_point(4, boot_info->modules_len);
	debug_point(5, boot_info->framebuffers_len);

	Katline::Syscalls::sys_memory_unmap(
	    boot_header_map, sizeof(Katline::BootInfoTable));

	Katline::Syscalls::sys_memory_unmap(boot_blob_map, boot_total_len);

	Katline::Syscalls::sys_handle_close(boot_handle);

	Katline::Syscalls::sys_exit(0);
	__builtin_trap();
}
