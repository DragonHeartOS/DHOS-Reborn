export module KatlineAPI:SyscallContract;

import CommonLib;
import :Types;
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
		constexpr auto get() const -> T * { return reinterpret_cast<T *>(raw); }
	};

	template<typename T> struct UserPtrConst {
		u64 raw {};

		static constexpr auto from_raw(u64 value) -> UserPtrConst
		{
			return UserPtrConst { value };
		}

		constexpr auto is_null() const -> bool { return raw == 0; }
		constexpr auto addr() const -> u64 { return raw; }
		constexpr auto get() const -> T const *
		{
			return reinterpret_cast<T const *>(raw);
		}
	};

	template<typename T>
	inline constexpr bool IsHandleV = CL::SameAs<CL::RemoveConstRef<T>, Handle>;

	}
}
