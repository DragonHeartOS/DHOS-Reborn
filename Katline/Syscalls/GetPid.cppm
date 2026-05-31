export module Katline:SyscallGetPid;

import CommonLib;
import :Scheduler;
import :SyscallTypes;

export {
	namespace Katline::Syscalls {
	auto get_pid() -> Result<u64>;
	}
}

namespace Katline::Syscalls {

auto get_pid() -> Result<u64>
{
	auto *thread { Arch::Scheduler::the().current_thread() };
	if (!thread || !thread->process)
		return Result<u64>::Err(ErrorsV::InvalidArgument {});
	return Result<u64>::Ok(thread->process->pid.id);
}

}
