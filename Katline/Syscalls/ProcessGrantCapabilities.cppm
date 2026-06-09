export module Katline:SyscallProcessGrantCapabilities;

import CommonLib;
import KatlineAPI;
import :HandleManager;
import :Scheduler;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ProcessGrantCapabilities> {
	static auto call(Handle process_handle, ProcessCapabilityFlags capabilities)
	    -> Result<void>
	{
		if (auto const res {
		        require_capability(ProcessCapability::ManageCaps),
		    };
		    res.is_err())
			return Result<void>::Err(res.unwrap_err());

		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

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
