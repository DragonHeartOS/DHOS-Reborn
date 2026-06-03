export module KatlineAPI:Syscalls;

import CommonLib;
import :Types;

export {
	namespace Katline::Syscalls {

	enum class SyscallNumber : u64 {
#define X(name, id, fn, ret, ...) name = id,
#include "SyscallList.def"
#undef X
	};

	namespace ErrorsV {
	struct InvalidSyscall { };
	struct InvalidArgument { };
	struct BadAddress { };
	struct PermissionDenied { };
	struct QueueFull { };
	struct EmptyQueue { };
	}

	using SyscallError = CL::Error<ErrorsV::InvalidSyscall,
	    ErrorsV::InvalidArgument, ErrorsV::BadAddress,
	    ErrorsV::PermissionDenied, ErrorsV::QueueFull, ErrorsV::EmptyQueue>;

	template<typename T> using Result = CL::Result<T, SyscallError>;

	auto encode_error(SyscallError const &error) -> u64;
	auto encode_void(Result<void> const &result) -> u64;
	auto encode_u64(Result<u64> const &result) -> u64;
	auto encode_handle(Result<Katline::Handle> const &result) -> u64;

	}
}

namespace Katline::Syscalls {

template<usize N>
consteval auto syscall_ids_are_unique(u64 const (&ids)[N]) -> bool
{
	for (usize i { 0 }; i < N; i++) {
		for (usize j { i + 1 }; j < N; j++) {
			if (ids[i] == ids[j])
				return false;
		}
	}

	return true;
}

constexpr u64 syscall_ids[] {
#define X(name, id, fn, ret, ...) static_cast<u64>(id),
#include "SyscallList.def"
#undef X
};

static_assert(syscall_ids_are_unique(syscall_ids),
    "duplicate syscall id in SyscallList.def");

namespace ErrorsV {

inline auto to_display_string(InvalidSyscall const &) -> CL::String
{
	return "invalid syscall";
}

inline auto to_display_string(InvalidArgument const &) -> CL::String
{
	return "invalid syscall argument";
}

inline auto to_display_string(BadAddress const &) -> CL::String
{
	return "bad userspace address";
}

inline auto to_display_string(PermissionDenied const &) -> CL::String
{
	return "syscall permission denied";
}

inline auto to_display_string(QueueFull const &) -> CL::String
{
	return "queue full";
}

inline auto to_display_string(EmptyQueue const &) -> CL::String
{
	return "empty queue";
}

}

auto encode_error(SyscallError const &error) -> u64
{
	return static_cast<u64>(-static_cast<i64>(error.tag() + 1));
}

auto encode_void(Result<void> const &result) -> u64
{
	if (result.is_err())
		return encode_error(result.unwrap_err());
	return 0;
}

auto encode_u64(Result<u64> const &result) -> u64
{
	if (result.is_err())
		return encode_error(result.unwrap_err());
	return *result;
}

auto encode_handle(Result<Katline::Handle> const &result) -> u64
{
	if (result.is_err())
		return encode_error(result.unwrap_err());
	return result->id;
}

}
