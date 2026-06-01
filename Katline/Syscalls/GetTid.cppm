export module Katline:SyscallGetTid;

import CommonLib;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

auto kernel_get_tid() -> Result<u64>
{
	auto *thread { Arch::Scheduler::the().current_thread() };
	if (!thread)
		return Result<u64>::Err(ErrorsV::InvalidArgument {});
	return Result<u64>::Ok(thread->tid.id);
}

template<> struct Spec<SyscallNumber::GetTid> {
	static auto call() -> Result<u64> { return kernel_get_tid(); }
};

}
