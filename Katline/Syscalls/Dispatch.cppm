export module Katline:SyscallDispatch;

import CommonLib;
import KatlineAPI;
import :Sync;

import :SyscallKernelContract;
import :SyscallExit;
import :SyscallProcessCurrent;
import :SyscallThreadCurrent;
import :SyscallYield;
import :SyscallIPCSend;
import :SyscallIPCReceive;
import :SyscallHandleClose;
import :SyscallHandleDuplicate;
import :SyscallGetProcessInfo;
import :SyscallProcessCreate;
import :SyscallThreadCreate;
import :SyscallThreadStart;
import :SyscallMemoryMap;
import :SyscallProcessMemoryMap;
import :SyscallMemoryUnmap;
import :SyscallProcessMemoryUnmap;
import :SyscallProcessGrantCapabilities;
import :SyscallMemoryCreate;
import :SyscallDebugU64;

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
	auto const irq_flags { Katline::Sync::irq_save_disable() };
	// TODO: replace with defer
	struct RestoreIrq {
		u64 flags;
		~RestoreIrq() { Katline::Sync::irq_restore(flags); }
	} restore { irq_flags };

	if (static_cast<SyscallNumber>(number) == SyscallNumber::Exit) {
		Katline::Syscalls::exit(static_cast<i32>(arg0));
		__builtin_unreachable();
	}

	switch (static_cast<SyscallNumber>(number)) {
#define X(name, id, fn, ret, ...) \
	case SyscallNumber::name: \
		return invoke_typed<SyscallNumber::name>(arg0, arg1, arg2, arg3, arg4);
#include "Katline/API/SyscallList.def"
#undef X
	default:
		return encode_error(SyscallError { ErrorsV::InvalidSyscall {} });
	}
}

}
