module CommonLib;

import :Platform;
import :StringView;
import Katline;

extern "C" [[noreturn]] auto commonlib_platform_panic(char const *message)
    -> void
{
	Katline::kpanic(CL::StringView(message));
}
