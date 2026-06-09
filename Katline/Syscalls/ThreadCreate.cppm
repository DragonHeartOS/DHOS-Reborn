export module Katline:SyscallThreadCreate;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ThreadCreate> {
	static auto call(Handle process_handle, u64 entry, u64 stack)
	    -> Result<Handle>
	{
		if (auto const res {
		        require_capability(ProcessCapability::ManageProcesses),
		    };
		    res.is_err())
			return Result<Handle>::Err(res.unwrap_err());

		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		auto *process {
			Arch::HandleManager::the().resolve<Arch::Process>(
			    thread->process, process_handle, Arch::HandleKind::Process),
		};
		if (!process || !entry || !stack
		    || !is_valid_user_address(static_cast<uptr>(entry))
		    || !is_valid_user_address(static_cast<uptr>(stack)))
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		auto *created {
			Arch::Scheduler::the().make_user_thread(
			    process, static_cast<uptr>(entry), static_cast<uptr>(stack)),
		};
		if (!created)
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		auto handle {
			Arch::HandleManager::the().open(
			    thread->process, Arch::HandleKind::Thread, created),
		};
		if (handle.is_invalid())
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		return Result<Handle>::Ok(handle);
	}
};

}
