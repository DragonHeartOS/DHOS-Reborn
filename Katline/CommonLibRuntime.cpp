module CommonLib;

import :Platform;
import Katline;

auto CL::panic(CL::StringView const message) -> void
{
	Katline::Debug::write_formatted(
	    "Kernel Panic: %.*s\n", message.size(), message.data());

	for (;;)
		asm("hlt");
}
