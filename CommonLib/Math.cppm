export module CommonLib:Math;

import :Types;

export {
	namespace CL::Math {

	template<typename T> auto median3(T a, T b, T c) -> T
	{
		if (a > b) {
			auto t { a };
			a = b;
			b = t;
		}
		if (b > c) {
			auto t { b };
			b = c;
			c = t;
		}
		if (a > b) {
			auto t { a };
			a = b;
			b = t;
		}
		return b;
	}

	class Point {
	public:
		Point(int y = 0, int x = 0)
		    : m_y { y }
		    , m_x { x }
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
}
