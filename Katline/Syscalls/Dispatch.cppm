export module Katline:SyscallDispatch;

import CommonLib;
import KatlineAPI;
import :SyscallKernelContract;

#define X(name, ...) import :Syscall##name;
#import <Katline/API/SyscallList.def>
#undef X

export {
	namespace Katline::Syscalls {

	extern "C" auto dispatch_raw(
	    u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) -> u64;

	}
}

namespace Katline::Syscalls {

extern "C" auto dispatch_raw(
    u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) -> u64
{
	CL::ignore_unused(arg0, arg1, arg2, arg3, arg4);
	switch (static_cast<SyscallNumber>(number)) {
#define X(name, id, fn, ret, ...) \
	case SyscallNumber::name: \
		return invoke_typed<SyscallNumber::name>(arg0, arg1, arg2, arg3, arg4);
#include <Katline/API/SyscallList.def>
#undef X
	default:
		break;
	}

	return encode_error(SyscallError { ErrorsV::InvalidSyscall {} });
}

}
