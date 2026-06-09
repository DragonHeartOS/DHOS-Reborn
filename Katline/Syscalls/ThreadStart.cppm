export module Katline:SyscallThreadStart;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ThreadStart> {
	static constexpr bool requires_current_thread = true;
	static constexpr ProcessCapabilityFlags required_capabilities {
		ProcessCapability::ManageProcesses
	};

	static auto call(Arch::Thread *thread, Handle thread_handle) -> Result<void>
	{
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
