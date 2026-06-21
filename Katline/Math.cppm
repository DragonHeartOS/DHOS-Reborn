export module Katline:Math;

export {
	namespace Katline::Math {

	template<typename T> struct Point {
		T m_x {};
		T m_y {};

		constexpr auto set_x(T value) -> void { m_x = value; }
		constexpr auto set_y(T value) -> void { m_y = value; }
		constexpr auto x() const -> T { return m_x; }
		constexpr auto y() const -> T { return m_y; }
	};

	}
}
