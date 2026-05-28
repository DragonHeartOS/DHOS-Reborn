#pragma once

#include <CommonLib/Types.h>

namespace CL::Math {

class Point {
public:
	Point(int y = 0, int x = 0)
	    : m_y(y)
	    , m_x(x)
	{
	}

	auto Y() -> int { return m_y; }
	auto X() -> int { return m_x; }

	auto SetY(int value) -> void { m_y = value; }
	auto SetX(int value) -> void { m_x = value; }

private:
	int m_y = 0;
	int m_x = 0;
};

}
