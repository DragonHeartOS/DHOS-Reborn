export module KatlineAPI:Types;

import CommonLib;

export {
	namespace Katline {

	struct Handle {
		u64 id {};

		constexpr auto is_invalid() const -> bool { return id == 0; }
	};

	struct ProcessID {
		u64 id;
	};

	struct ThreadID {
		u64 id;
	};

	struct ProcessInfo {
		ProcessID pid;
		u64 handle_count;
		u64 name_len;
		char const name[];
	};

	}
}
