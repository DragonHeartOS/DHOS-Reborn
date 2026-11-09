module CommonLib;

import :Platform;

extern "C" [[noreturn]] auto commonlib_platform_panic(char const *message)
    -> void
{
	(void)message;
	__builtin_trap();
}
