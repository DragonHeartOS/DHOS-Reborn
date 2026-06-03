export module Katline:SyscallHandleClose;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::HandleClose> {
	static auto call(Handle handle) -> Result<void>
	{
		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		if (!Arch::HandleManager::the().close(thread->process, handle))
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		return Result<void>::Ok();
	}
};

}
