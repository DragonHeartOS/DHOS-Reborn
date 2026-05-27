#pragma once

#include <CommonLib/Math.h>
#include <Katline/Controllers/IO.h>

#include <stdarg.h>

namespace Katline {

namespace Controller {

uint const PORT = 0x3f8;

class SerialController {
public:
    SerialController() { };
    bool Init();
    int Received();
    char Read();
    int IsTransmitEmpty();
    void Write(char ch);
    void WriteStringSafe(char const* string, size_t size);
    void WriteString(char const* string);

private:
    bool m_enabled = false;
};

}

}
