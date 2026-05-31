export module Katline:SyscallDispatch;

import CommonLib;
import :SyscallTypes;

export {
	namespace Katline::Syscalls {

	extern "C" auto dispatch_raw(
	    u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) -> u64;

	}
}

namespace Katline::Syscalls {

auto get_pid() -> Result<u64>;
auto get_tid() -> Result<u64>;
auto yield() -> Result<void>;
[[noreturn]] auto exit(i32 code) -> void;

extern "C" auto dispatch_raw(
    u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) -> u64
{
	CL::ignore_unused(arg1, arg2, arg3, arg4);

	switch (static_cast<Number>(number)) {
	case Number::GetPid:
		return encode_u64(get_pid());
	case Number::GetTid:
		return encode_u64(get_tid());
	case Number::Yield:
		return encode_void(yield());
	case Number::Exit:
		exit(static_cast<i32>(arg0));
	}

	return encode_error(SyscallError { ErrorsV::InvalidSyscall {} });
}

}
