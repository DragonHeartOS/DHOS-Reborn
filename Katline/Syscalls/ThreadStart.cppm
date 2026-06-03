export module Katline:SyscallThreadStart;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ThreadStart> {
	static auto call(Handle thread_handle) -> Result<void>
	{
		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		auto *target {
			Arch::HandleManager::the().resolve<Arch::Thread>(
			    thread->process, thread_handle, Arch::HandleKind::Thread),
		};
		if (!target)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		if (!Arch::Scheduler::the().start_thread(target))
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		return Result<void>::Ok();
	}
};

}
