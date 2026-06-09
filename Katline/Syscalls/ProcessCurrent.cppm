export module Katline:SyscallProcessCurrent;

import CommonLib;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ProcessCurrent> {
	static constexpr bool requires_current_thread = true;

	static auto call(Arch::Thread *thread) -> Result<Handle>
	{
		auto handle {
			Arch::HandleManager::the().open(
			    thread->process, Arch::HandleKind::Process, thread->process),
		};
		if (handle.is_invalid())
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		return Result<Handle>::Ok(handle);
	}
};

}
