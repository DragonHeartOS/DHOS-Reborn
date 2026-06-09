export module Katline:SyscallThreadCurrent;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ThreadCurrent> {
	static constexpr bool requires_current_thread = true;

	static auto call(Arch::Thread *thread) -> Result<Handle>
	{
		auto handle { Arch::HandleManager::the().open(
			thread->process, Arch::HandleKind::Thread, thread) };
		if (handle.is_invalid())
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		return Result<Handle>::Ok(handle);
	}
};

}
