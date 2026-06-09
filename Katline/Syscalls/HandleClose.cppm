export module Katline:SyscallHandleClose;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::HandleClose> {
	static constexpr bool requires_current_thread = true;

	static auto call(Arch::Thread *thread, Handle handle) -> Result<void>
	{
		if (!Arch::HandleManager::the().close(thread->process, handle))
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		return Result<void>::Ok();
	}
};

}
