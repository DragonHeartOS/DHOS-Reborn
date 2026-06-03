export module Katline:SyscallProcessCurrent;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ProcessCurrent> {
	static auto call() -> Result<Handle>
	{
		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		auto handle {
			Arch::HandleManager::the().open(
			    thread->process, Arch::HandleKind::Process, thread->process),
		};
		if (handle.is_invalid())
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		return Result<Handle>::Ok(handle);
	}
};

}
