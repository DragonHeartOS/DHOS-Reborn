export module Katline:SyscallHandleDuplicate;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::HandleDuplicate> {
	static constexpr bool requires_current_thread = true;

	static auto call(Arch::Thread *thread, Handle handle) -> Result<Handle>
	{
		auto duplicate {
			Arch::HandleManager::the().duplicate(thread->process, handle),
		};
		if (duplicate.is_invalid())
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		return Result<Handle>::Ok(duplicate);
	}
};

}
