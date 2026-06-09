export module Katline:SyscallThreadCreate;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ThreadCreate> {
	static constexpr bool requires_current_thread = true;
	static constexpr ProcessCapabilityFlags required_capabilities {
		ProcessCapability::ManageProcesses
	};

	static auto call(Arch::Thread *thread, Handle process_handle, u64 entry,
	    u64 stack) -> Result<Handle>
	{
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
