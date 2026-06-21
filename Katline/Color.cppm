export module Katline:Color;

import CommonLib;

export {
	namespace Katline::Color {

	struct RGBColor {
		u8 r {};
		u8 g {};
		u8 b {};
	};

	inline constexpr RGBColor BLACK { 0, 0, 0 };
	inline constexpr RGBColor WHITE { 255, 255, 255 };

	}
}
