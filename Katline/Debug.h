#pragma once

#include <stdarg.h>

namespace Katline {

namespace Debug {

void set_framebuffer_logging_enabled(bool enabled);
void write_formatted(char const* str, ...);

}

}
