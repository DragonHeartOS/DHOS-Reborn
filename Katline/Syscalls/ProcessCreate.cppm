export module Katline:SyscallProcessCreate;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ProcessCreate> {
	static auto call() -> Result<Handle>
	{
		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		auto *process {
			Arch::Scheduler::the().create_user_process(thread->process),
		};
		if (!process)
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		auto handle {
			Arch::HandleManager::the().open(
			    thread->process, Arch::HandleKind::Process, process),
		};
		if (handle.is_invalid())
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		return Result<Handle>::Ok(handle);
	}
};

}
