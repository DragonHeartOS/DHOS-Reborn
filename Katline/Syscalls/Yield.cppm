export module Katline:SyscallYield;

import CommonLib;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

export {
	namespace Katline::Syscalls {
	auto yield() -> Result<void>;
	}
}

namespace Katline::Syscalls {

auto yield() -> Result<void>
{
	Arch::Scheduler::the().yield();
	return Result<void>::Ok();
}

template<> struct Spec<SyscallNumber::Yield> {
	static auto call() -> Result<void> { return yield(); }
};

}
