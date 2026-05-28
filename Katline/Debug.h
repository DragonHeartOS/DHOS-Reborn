#pragma once

#include <stdarg.h>

namespace Katline {

namespace Debug {

void set_framebuffer_logging_enabled(bool enabled);
void print_formatted(char const *str, ...);
void write_formatted(char const *str, ...);

}

}
