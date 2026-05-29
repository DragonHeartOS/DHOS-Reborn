module CommonLib;

import :Platform;
import Katline;

auto CL::panic(CL::StringView const message) -> void
{
	Katline::kpanic(message);
}
