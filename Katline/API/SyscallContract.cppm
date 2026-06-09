export module KatlineAPI:SyscallContract;

import CommonLib;
import :Types;
import :Syscalls;

export {
	namespace Katline::Syscalls {

	template<typename T> struct UserPtr {
		constexpr UserPtr() = default;
		constexpr UserPtr(nullptr_t)
		    : raw(0)
		{
		}
		constexpr UserPtr(T *ptr)
		    : raw(reinterpret_cast<u64>(ptr))
		{
		}

		u64 raw {};

		constexpr auto operator=(nullptr_t) -> UserPtr &
		{
			raw = 0;
			return *this;
		}

		static constexpr auto from_raw(u64 value) -> UserPtr
		{
			return UserPtr { value };
		}

		constexpr auto is_null() const -> bool { return raw == 0; }
		constexpr auto addr() const -> u64 { return raw; }
		constexpr auto get() const -> T * { return reinterpret_cast<T *>(raw); }
	};

	template<typename T> struct UserPtrConst {
		constexpr UserPtrConst() = default;
		constexpr UserPtrConst(nullptr_t)
		    : raw(0)
		{
		}
		constexpr UserPtrConst(T *ptr)
		    : raw(reinterpret_cast<u64>(ptr))
		{
		}
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

	template<typename T> struct IsUserPointer {
		static constexpr bool Value = false;
	};

	template<typename T> struct IsUserPointer<UserPtr<T>> {
		static constexpr bool Value = true;
	};

	template<typename T> struct IsUserPointer<UserPtrConst<T>> {
		static constexpr bool Value = true;
	};

	template<typename T>
	inline constexpr bool IsUserPointerV = IsUserPointer<T>::Value;

	template<typename T> struct IsFlags {
		static constexpr bool Value = false;
	};

	template<typename Enum> struct IsFlags<CL::Flags<Enum>> {
		static constexpr bool Value = true;
	};

	template<typename T>
	inline constexpr bool IsFlagsV = IsFlags<CL::RemoveConstRef<T>>::Value;

	template<typename T>
	inline constexpr bool IsSyscallArgV = CL::IsIntegralV<T> || CL::IsEnumV<T>
	    || IsUserPointerV<T> || IsHandleV<T> || IsFlagsV<T>;

	}
}
