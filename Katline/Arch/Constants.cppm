export module Katline:ArchConstants;

import CommonLib;

export {
	namespace Katline::Arch {

	inline constexpr usize k_page_size { 4096 };
	inline constexpr usize k_page_mask { k_page_size - 1 };
	inline constexpr uptr k_user_space_limit { 0x0000800000000000ull };

	}
}
