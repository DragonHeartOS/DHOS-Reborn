export module Katline:SyscallKernelContract;

import CommonLib;
import KatlineAPI;

export {
	namespace Katline::Syscalls {

	template<SyscallNumber Id> struct Spec;

	template<SyscallNumber Id>
	auto invoke_typed(u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) -> u64;

	}
}

namespace Katline::Syscalls {

template<SyscallNumber Id> struct ReturnOf;
template<SyscallNumber Id> struct ArgsOf;

#define X(name, id, fn, ret, ...) \
	template<> struct ReturnOf<SyscallNumber::name> { \
		using Type = ret; \
	}; \
	template<> struct ArgsOf<SyscallNumber::name> { \
		using Type = CL::TypeList<__VA_ARGS__>; \
	};
#include <Katline/API/SyscallList.def>
#undef X

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

template<usize Index>
constexpr auto raw_arg_at(u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4)
    -> u64
{
	static_assert(Index < 5, "syscall argument index out of range");
	if constexpr (Index == 0)
		return arg0;
	if constexpr (Index == 1)
		return arg1;
	if constexpr (Index == 2)
		return arg2;
	if constexpr (Index == 3)
		return arg3;
	return arg4;
}

template<typename T> constexpr auto decode_syscall_arg(u64 value) -> T
{
	static_assert(IsSyscallArgV<T>, "unsupported syscall argument type");

	if constexpr (IsUserPointerV<T>) {
		return T::from_raw(value);
	} else {
		return static_cast<T>(value);
	}
}

template<typename Ret> auto encode_result(Result<Ret> const &result) -> u64
{
	if constexpr (CL::SameAs<Ret, void>)
		return encode_void(result);
	else
		return encode_u64(result);
}

template<SyscallNumber Id, usize... I>
auto invoke_typed_impl(u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4,
    CL::IndexSequence<I...>) -> u64
{
	using S = Spec<Id>;
	using Args = typename ArgsOf<Id>::Type;
	using Return = typename ReturnOf<Id>::Type;

	return encode_result<Return>(
	    S::call(decode_syscall_arg<CL::TypeListAtT<I, Args>>(
	        raw_arg_at<I>(arg0, arg1, arg2, arg3, arg4))...));
}

template<SyscallNumber Id, usize... I>
constexpr auto has_valid_call_signature(CL::IndexSequence<I...>) -> bool
{
	using S = Spec<Id>;
	using Args = typename ArgsOf<Id>::Type;
	using Return = typename ReturnOf<Id>::Type;

	return requires {
		{
			S::call(CL::declval<CL::TypeListAtT<I, Args>>()...)
		} -> CL::SameAs<Result<Return>>;
	};
}

template<SyscallNumber Id>
auto invoke_typed(u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) -> u64
{
	constexpr bool has_spec { requires { &Spec<Id>::call;
}
};

if constexpr (!has_spec) {
	static_assert(has_spec,
	    "Missing syscall implementation for SyscallList.def entry. "
	    "Add template<> struct Spec<SyscallNumber::X> with call(...).");
	return 0;
} else {
	using Args = typename ArgsOf<Id>::Type;

	static_assert(CL::IsTypeListV<Args>,
	    "SyscallList.def arguments must be a valid type list");

	constexpr usize arg_count { CL::TypeListSizeV<Args> };
	static_assert(arg_count <= 5, "syscall argument count exceeds ABI limit");

	static_assert(
	    has_valid_call_signature<Id>(CL::MakeIndexSequence<arg_count> {}),
	    "Spec<...>::call signature must match SyscallList.def args and return "
	    "type");

	return invoke_typed_impl<Id>(
	    arg0, arg1, arg2, arg3, arg4, CL::MakeIndexSequence<arg_count> {});
}
}
}
