module;

#include <stdarg.h>

export module Katline:Debug;

export {
	namespace Katline {

	namespace Debug {

	auto init_log_queue() -> void;
	void set_framebuffer_logging_enabled(bool enabled);
	auto drain_logs() -> void;
	void print_formatted(char const *str, ...);
	void write_formatted(char const *str, ...);

	}

	}
}
