import CommonLib;
import KatlineAPI;

extern "C" [[noreturn]] auto _start() -> void
{
	auto const memory_res { Katline::Syscalls::sys_memory_create(4096) };
	if (memory_res.is_err())
		Katline::Syscalls::sys_exit(1);

	void *mapped_dummy {};
	auto const map_res {
		Katline::Syscalls::sys_memory_map(*memory_res, 0, 4096,
		    Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable },
		    &mapped_dummy),
	};
	if (map_res.is_err())
		Katline::Syscalls::sys_exit(2);
	auto const mapped { reinterpret_cast<void *>(*map_res) };

	if (!mapped)
		Katline::Syscalls::sys_exit(3);

	auto *words { static_cast<unsigned long long *>(mapped) };
	words[0] = 0x1122334455667788ull;
	if (words[0] != 0x1122334455667788ull)
		Katline::Syscalls::sys_exit(4);

	auto const unmap_res { Katline::Syscalls::sys_memory_unmap(mapped, 4096) };
	if (unmap_res.is_err())
		Katline::Syscalls::sys_exit(5);

	auto const close_res { Katline::Syscalls::sys_handle_close(*memory_res) };
	if (close_res.is_err())
		Katline::Syscalls::sys_exit(6);

	Katline::Syscalls::sys_exit(0);
	__builtin_trap(); // HACK: make the compielr shut up, should probably find a
	                  // way to make the sys_exit have [[noreturn]] as well.
}
