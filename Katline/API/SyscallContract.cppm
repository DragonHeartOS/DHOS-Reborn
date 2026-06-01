export module KatlineAPI:SyscallContract;

import CommonLib;
import :Syscalls;

export {
	namespace Katline::Syscalls {

	template<typename T> struct UserPtr {
		u64 raw {};

		static constexpr auto from_raw(u64 value) -> UserPtr
		{
			return UserPtr { value };
		}

		constexpr auto is_null() const -> bool { return raw == 0; }
		constexpr auto addr() const -> u64 { return raw; }
	};

	template<typename T> struct UserPtrConst {
		u64 raw {};

		static constexpr auto from_raw(u64 value) -> UserPtrConst
		{
			return UserPtrConst { value };
		}

		constexpr auto is_null() const -> bool { return raw == 0; }
		constexpr auto addr() const -> u64 { return raw; }
	};

	}
}
