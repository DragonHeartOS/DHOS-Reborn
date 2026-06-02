export module KatlineAPI:SyscallCalls;

import CommonLib;
import :Syscalls;
import :SyscallContract;
import :IPC;

export {
	namespace Katline::Syscalls {

	extern "C" auto syscall_raw(
	    u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) -> u64;

#define X(name, id, fn, ret, ...) auto fn(__VA_ARGS__)->Result<ret>;
#include "SyscallList.def"
#undef X

	}
}

namespace Katline::Syscalls {

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

template<typename T>
inline constexpr bool IsSyscallArgV
    = CL::IsIntegralV<T> || CL::IsEnumV<T> || IsUserPointerV<T>;

template<typename T> constexpr auto encode_syscall_arg(T value) -> u64
{
	static_assert(IsSyscallArgV<T>, "unsupported syscall argument type");

	if constexpr (IsUserPointerV<T>) {
		return value.addr();
	} else {
		return static_cast<u64>(value);
	}
}

template<typename Ret> auto decode_syscall_result(u64 raw) -> Result<Ret>
{
	if (static_cast<i64>(raw) < 0) {
		auto const error_index { static_cast<usize>(
			-static_cast<i64>(raw) - 1) };
		return Result<Ret>::Err(SyscallError::from_tag_unsafe(error_index));
	}

	if constexpr (CL::SameAs<Ret, void>) {
		return Result<void>::Ok();
	} else {
		static_assert(CL::SameAs<Ret, u64>,
		    "only void and u64 syscall return types are currently supported");
		return Result<u64>::Ok(raw);
	}
}

template<typename Ret, typename... Args>
auto invoke_userspace(SyscallNumber id, Args... args) -> Result<Ret>
{
	static_assert(
	    sizeof...(Args) <= 5, "syscall argument count exceeds ABI limit");
	return decode_syscall_result<Ret>(syscall_raw(static_cast<u64>(id),
	    (sizeof...(Args) > 0 ? encode_syscall_arg(args) : 0)..., 0, 0, 0, 0,
	    0));
}

template<typename Ret> auto invoke_userspace(SyscallNumber id) -> Result<Ret>
{
	return decode_syscall_result<Ret>(
	    syscall_raw(static_cast<u64>(id), 0, 0, 0, 0, 0));
}

template<typename Ret, typename A0>
auto invoke_userspace(SyscallNumber id, A0 a0) -> Result<Ret>
{
	return decode_syscall_result<Ret>(
	    syscall_raw(static_cast<u64>(id), encode_syscall_arg(a0), 0, 0, 0, 0));
}

template<typename Ret, typename A0, typename A1>
auto invoke_userspace(SyscallNumber id, A0 a0, A1 a1) -> Result<Ret>
{
	return decode_syscall_result<Ret>(syscall_raw(static_cast<u64>(id),
	    encode_syscall_arg(a0), encode_syscall_arg(a1), 0, 0, 0));
}

template<typename Ret, typename A0, typename A1, typename A2>
auto invoke_userspace(SyscallNumber id, A0 a0, A1 a1, A2 a2) -> Result<Ret>
{
	return decode_syscall_result<Ret>(
	    syscall_raw(static_cast<u64>(id), encode_syscall_arg(a0),
	        encode_syscall_arg(a1), encode_syscall_arg(a2), 0, 0));
}

template<typename Ret, typename A0, typename A1, typename A2, typename A3>
auto invoke_userspace(SyscallNumber id, A0 a0, A1 a1, A2 a2, A3 a3)
    -> Result<Ret>
{
	return decode_syscall_result<Ret>(syscall_raw(static_cast<u64>(id),
	    encode_syscall_arg(a0), encode_syscall_arg(a1), encode_syscall_arg(a2),
	    encode_syscall_arg(a3), 0));
}

template<typename Ret, typename A0, typename A1, typename A2, typename A3,
    typename A4>
auto invoke_userspace(SyscallNumber id, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4)
    -> Result<Ret>
{
	return decode_syscall_result<Ret>(syscall_raw(static_cast<u64>(id),
	    encode_syscall_arg(a0), encode_syscall_arg(a1), encode_syscall_arg(a2),
	    encode_syscall_arg(a3), encode_syscall_arg(a4)));
}

#define X(name, id, fn, ret, ...) \
	template<typename... A> auto fn(A... args) -> Result<ret> \
	{ \
		static_assert( \
		    CL::SameAs<CL::TypeList<A...>, CL::TypeList<__VA_ARGS__>>, \
		    "syscall wrapper argument types must match SyscallList.def"); \
		return invoke_userspace<ret>(SyscallNumber::name, args...); \
	}
#include "SyscallList.def"
#undef X

}
