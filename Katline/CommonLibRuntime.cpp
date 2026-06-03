module CommonLib;

import :Platform;
import :StringView;
import Katline;

auto CL::panic(char const *message) -> void
{
	Katline::kpanic(CL::StringView(message));
}
