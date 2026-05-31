export module Katline:SyscallTypes;

import CommonLib;

export {
	namespace Katline::Syscalls {

	enum class Number : u64 {
		GetPid = 0,
		GetTid = 1,
		Yield = 2,
		Exit = 3,
	};

	namespace ErrorsV {
	struct InvalidSyscall { };
	struct InvalidArgument { };
	struct BadAddress { };
	struct PermissionDenied { };
	}

	using SyscallError
	    = CL::Error<ErrorsV::InvalidSyscall, ErrorsV::InvalidArgument,
	        ErrorsV::BadAddress, ErrorsV::PermissionDenied>;

	template<typename T> using Result = CL::Result<T, SyscallError>;

	auto encode_error(SyscallError const &error) -> u64;
	auto encode_void(Result<void> const &result) -> u64;
	auto encode_u64(Result<u64> const &result) -> u64;

	}
}

namespace Katline::Syscalls::ErrorsV {

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

}

namespace Katline::Syscalls {

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

}
