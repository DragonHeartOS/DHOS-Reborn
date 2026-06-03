export module Katline:SyscallHandleDuplicate;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::HandleDuplicate> {
	static auto call(Handle handle) -> Result<Handle>
	{
		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		auto duplicate {
			Arch::HandleManager::the().duplicate(thread->process, handle),
		};
		if (duplicate.is_invalid())
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		return Result<Handle>::Ok(duplicate);
	}
};

}
