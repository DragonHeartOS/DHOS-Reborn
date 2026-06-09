export module Katline:SyscallProcessGrantCapabilities;

import CommonLib;
import KatlineAPI;
import :HandleManager;
import :Scheduler;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ProcessGrantCapabilities> {
	static constexpr bool requires_current_thread = true;
	static constexpr ProcessCapabilityFlags required_capabilities {
		ProcessCapability::ManageCaps
	};

	static auto call(Arch::Thread *thread, Handle process_handle,
	    ProcessCapabilityFlags capabilities) -> Result<void>
	{
		auto *target_process {
			Arch::HandleManager::the().resolve<Arch::Process>(
			    thread->process, process_handle, Arch::HandleKind::Process),
		};
		if (!target_process || target_process->parent != thread->process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		if (!thread->process->capabilities.contains_all(capabilities))
			return Result<void>::Err(ErrorsV::MissingCapability {});

		target_process->capabilities += capabilities;
		return Result<void>::Ok();
	}
};

}
