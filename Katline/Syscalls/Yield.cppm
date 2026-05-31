export module Katline:SyscallYield;

import CommonLib;
import :Scheduler;
import :SyscallTypes;

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

}
