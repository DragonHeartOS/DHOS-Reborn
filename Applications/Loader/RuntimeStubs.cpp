extern "C" [[noreturn]] auto commonlib_platform_panic(char const *) -> void
{
	__builtin_trap();
}
