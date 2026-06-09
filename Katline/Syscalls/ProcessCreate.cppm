export module Katline:SyscallProcessCreate;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ProcessCreate> {
	static constexpr bool requires_current_thread = true;
	static constexpr ProcessCapabilityFlags required_capabilities {
		ProcessCapability::ManageProcesses
	};

	static auto call(Arch::Thread *thread) -> Result<Handle>
	{
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
