module;

#include <stdarg.h>

export module Katline:Debug;

export {
	namespace Katline {

	namespace Debug {

	void set_framebuffer_logging_enabled(bool enabled);
	void print_formatted(char const *str, ...);
	void write_formatted(char const *str, ...);

	}

	}
}
