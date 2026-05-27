#pragma once

#include <CommonLib/Types.h>

namespace CL::Math {

class Point {
public:
    Point(int y = 0, int x = 0)
        : m_y(y)
        , m_x(x) { };

    int Y() { return m_y; }
    int X() { return m_x; }

    void SetY(int value) { m_y = value; }
    void SetX(int value) { m_x = value; }

private:
    int m_y = 0;
    int m_x = 0;
};

}
