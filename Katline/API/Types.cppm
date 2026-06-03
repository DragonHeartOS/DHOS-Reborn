export module KatlineAPI:Types;

import CommonLib;

export {
	namespace Katline {

	struct Handle {
		u64 id {};

		constexpr auto is_invalid() const -> bool { return id == 0; }
	};

	}
}
