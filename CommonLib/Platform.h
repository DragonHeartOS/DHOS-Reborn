#pragma once

#include <CommonLib/StringView.h>

namespace CL {

[[noreturn]] void panic(StringView const message);

}
