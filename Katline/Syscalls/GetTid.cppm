export module Katline:SyscallGetTid;

import CommonLib;
import :Scheduler;
import :SyscallTypes;

export {
	namespace Katline::Syscalls {
	auto get_tid() -> Result<u64>;
	}
}

namespace Katline::Syscalls {

auto get_tid() -> Result<u64>
{
	auto *thread { Arch::Scheduler::the().current_thread() };
	if (!thread)
		return Result<u64>::Err(ErrorsV::InvalidArgument {});
	return Result<u64>::Ok(thread->tid.id);
}

}
