#pragma once

#include <stdarg.h>

namespace Katline {

namespace Debug {

void SetFramebufferLoggingEnabled(bool enabled);
void WriteFormatted(char const* str, ...);

}

}
