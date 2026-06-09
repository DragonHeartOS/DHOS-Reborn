export module Katline:SyscallKernelContract;

import CommonLib;
import KatlineAPI;
import :Paging;
import :HandleManager;
import :Scheduler;

export {
	namespace Katline::Syscalls {

	constexpr auto is_valid_user_address(uptr addr) -> bool
	{
		return addr < Arch::k_user_space_limit && (addr >> 48) == 0;
	}

	template<typename T>
	auto copy_from_user(uptr raw, T *buffer, usize size) -> bool
	{
		if (size == 0)
			return true;

		auto *root { Arch::Paging::current_root() };
		usize copied {};
		while (copied < size) {
			auto const virt { raw + copied };
			auto const page { Arch::Paging::query(root, virt) };
			if (!page)
				return false;

			if (!page->flags.contains(Arch::Paging::PageFlag::User))
				return false;

			auto const page_offset { virt % page->page_size };
			auto const chunk {
				page->page_size - page_offset < size - copied
				    ? page->page_size - page_offset
				    : size - copied,
			};
			auto *const user_ptr { reinterpret_cast<u8 *>(
				Arch::Paging::phys_to_virt(page->physical)) };

			CL::memcpy(reinterpret_cast<u8 *>(buffer) + copied,
			    user_ptr + page_offset, chunk);

			copied += chunk;
		}

		return true;
	}

	template<typename T>
	auto copy_to_user(uptr raw, T const *buffer, usize size) -> bool
	{
		if (size == 0)
			return true;

		auto *root { Arch::Paging::current_root() };
		usize copied {};
		while (copied < size) {
			auto const virt { raw + copied };
			auto const page { Arch::Paging::query(root, virt) };
			if (!page)
				return false;

			if (!page->flags.contains(Arch::Paging::PageFlag::User))
				return false;
			if (!page->flags.contains(Arch::Paging::PageFlag::Writable))
				return false;

			auto const page_offset { virt % page->page_size };
			auto const chunk {
				page->page_size - page_offset < size - copied
				    ? page->page_size - page_offset
				    : size - copied,
			};
			auto *const user_ptr { reinterpret_cast<u8 *>(
				Arch::Paging::phys_to_virt(page->physical)) };

			CL::memcpy(user_ptr + page_offset,
			    reinterpret_cast<u8 const *>(buffer) + copied, chunk);

			copied += chunk;
		}

		return true;
	}

	template<typename T> auto copy_in(UserPtrConst<T> ptr) -> Result<T>
	{
		static_assert(__is_trivially_copyable(T),
		    "userspace copies require trivially copyable types");

		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<T>::Err(ErrorsV::InvalidArgument {});

		auto const raw { ptr.addr() };
		if (ptr.is_null() || sizeof(T) > Arch::k_user_space_limit
		    || !is_valid_user_address(raw)
		    || raw > Arch::k_user_space_limit - sizeof(T))
			return Result<T>::Err(ErrorsV::BadAddress {});

		T value {};
		if (!copy_from_user(raw, &value, sizeof(T)))
			return Result<T>::Err(ErrorsV::BadAddress {});

		return Result<T>::Ok(value);
	}

	template<typename T>
	auto copy_out(UserPtr<T> ptr, T const &value) -> Result<void>
	{
		static_assert(__is_trivially_copyable(T),
		    "userspace copies require trivially copyable types");

		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		auto const raw { ptr.addr() };
		if (ptr.is_null() || sizeof(T) > Arch::k_user_space_limit
		    || !is_valid_user_address(raw)
		    || raw > Arch::k_user_space_limit - sizeof(T))
			return Result<void>::Err(ErrorsV::BadAddress {});

		if (!copy_to_user(raw, &value, sizeof(T)))
			return Result<void>::Err(ErrorsV::BadAddress {});

		return Result<void>::Ok();
	}

	template<typename T>
	inline auto copy_out(UserPtr<T> ptr, void const *value, usize size)
	    -> Result<void>
	{
		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		auto const raw { ptr.addr() };
		if (ptr.is_null() || size > Arch::k_user_space_limit
		    || !is_valid_user_address(raw)
		    || raw > Arch::k_user_space_limit - size)
			return Result<void>::Err(ErrorsV::BadAddress {});

		if (size != 0 && !value)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		if (!copy_to_user(raw, reinterpret_cast<u8 const *>(value), size))
			return Result<void>::Err(ErrorsV::BadAddress {});

		return Result<void>::Ok();
	}

	template<typename T>
	auto resolve_handle(Handle handle, Arch::HandleKind kind) -> T *
	{
		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return nullptr;

		return Arch::HandleManager::the().resolve<T>(
		    thread->process, handle, kind);
	}

	inline auto validate_handle(Handle handle, Arch::HandleKind kind) -> bool
	{
		return resolve_handle<u8>(handle, kind) != nullptr;
	}

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
#include "Katline/API/SyscallList.def"
#undef X

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
	} else if constexpr (CL::SameAs<CL::RemoveConstRef<T>, Handle>) {
		return Handle { value };
	} else if constexpr (IsFlagsV<T>) {
		return T { static_cast<typename CL::RemoveConstRef<T>::Underlying>(
			value) };
	} else {
		return static_cast<T>(value);
	}
}

template<typename Ret> auto encode_result(Result<Ret> const &result) -> u64
{
	if constexpr (CL::SameAs<Ret, void>)
		return encode_void(result);
	else if constexpr (CL::SameAs<Ret, Handle>)
		return encode_handle(result);
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
