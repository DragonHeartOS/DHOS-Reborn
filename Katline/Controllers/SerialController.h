#pragma once

#include <CommonLib/Math.h>
#include <Katline/Controllers/IO.h>

#include <stdarg.h>

namespace Katline {

namespace Controller {

constexpr uint PORT { 0x3f8 };

class SerialController {
public:
    SerialController() { };

    bool init();
    int received();
    char read();
    int is_transmit_empty();
    void write(char ch);
    void write_string_safe(char const* string, size_t size);
    void write_string(char const* string);

private:
    bool m_enabled = false;
};

}

}
