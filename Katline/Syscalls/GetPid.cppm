export module Katline:SyscallGetPid;

import CommonLib;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

auto kernel_get_pid() -> Result<u64>
{
	auto *thread { Arch::Scheduler::the().current_thread() };
	if (!thread || !thread->process)
		return Result<u64>::Err(ErrorsV::InvalidArgument {});
	return Result<u64>::Ok(thread->process->pid.id);
}

template<> struct Spec<SyscallNumber::GetPid> {
	static auto call() -> Result<u64> { return kernel_get_pid(); }
};

}
