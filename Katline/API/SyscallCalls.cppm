export module KatlineAPI:SyscallCalls;

import CommonLib;
import :Types;
import :Syscalls;
import :SyscallContract;
import :IPC;

export {
	namespace Katline::Syscalls {

	extern "C" auto syscall_raw(
	    u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) -> u64;
	}
}

namespace Katline::Syscalls {

template<typename ExpectedList, typename ActualList>
struct AreSyscallArgsCompatible;

template<typename... Expected, typename... Actual>
struct AreSyscallArgsCompatible<CL::TypeList<Expected...>, CL::TypeList<Actual...>> {
	static constexpr bool Value = sizeof...(Expected) == sizeof...(Actual)
	    && (__is_constructible(Expected, Actual) && ...);
};

template<typename ExpectedList, typename ActualList>
inline constexpr bool AreSyscallArgsCompatibleV
	= AreSyscallArgsCompatible<ExpectedList, ActualList>::Value;

template<typename Ret, typename... Expected, typename... Actual>
auto invoke_userspace_typed(
	SyscallNumber id, CL::TypeList<Expected...>, Actual... args) -> Result<Ret>
{
	static_assert(sizeof...(Expected) == sizeof...(Actual));
	return invoke_userspace<Ret>(id, Expected(args)...);
}

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
    = CL::IsIntegralV<T> || CL::IsEnumV<T> || IsUserPointerV<T> || IsHandleV<T>;

template<typename T> constexpr auto encode_syscall_arg(T value) -> u64
{
	static_assert(IsSyscallArgV<T>, "unsupported syscall argument type");

	if constexpr (IsHandleV<T>) {
		return value.id;
	} else if constexpr (IsUserPointerV<T>) {
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
	} else if constexpr (CL::SameAs<Ret, Handle>) {
		return Result<Handle>::Ok(Handle { raw });
	} else {
		static_assert(CL::SameAs<Ret, u64>,
		    "only void, u64, and Handle syscall return types are currently "
		    "supported");
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

extern "C" auto syscall_raw(
    u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) -> u64
{
	register u64 rax asm("rax") = number;
	register u64 rdi asm("rdi") = arg0;
	register u64 rsi asm("rsi") = arg1;
	register u64 rdx asm("rdx") = arg2;
	register u64 r10 asm("r10") = arg3;
	register u64 r8 asm("r8") = arg4;

	asm volatile("syscall"
	    : "+a"(rax)
	    : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8)
	    : "rcx", "r11", "memory");

	return rax;
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
	export template<typename... A> auto fn(A... args) -> Result<ret> \
	{ \
		using ExpectedArgs = CL::TypeList<__VA_ARGS__>; \
		static_assert( \
		    AreSyscallArgsCompatibleV<ExpectedArgs, CL::TypeList<A...>>, \
		    "syscall wrapper argument types must be constructible to those declared in SyscallList.def"); \
		return invoke_userspace_typed<ret>(SyscallNumber::name, ExpectedArgs {}, args...); \
	}
#include "SyscallList.def"
#undef X

}
