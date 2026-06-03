module CommonLib;

import :Platform;

auto CL::panic(char const *message) -> void
{
	(void)message;
	__builtin_trap();
}
