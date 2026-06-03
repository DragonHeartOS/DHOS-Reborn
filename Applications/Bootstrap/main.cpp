import CommonLib;
import KatlineAPI;

extern "C" [[noreturn]] auto _start() -> void
{
	Katline::Syscalls::sys_exit(0);
	__builtin_trap(); // HACK: make the compielr shut up, should probably find a
	                  // way to make the sys_exit have [[noreturn]] as well.
}
