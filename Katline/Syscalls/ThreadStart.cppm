export module Katline:SyscallThreadStart;

import CommonLib;
import :Debug;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ThreadStart> {
	static constexpr bool requires_current_thread = true;

	static auto call(Arch::Thread *thread, Handle thread_handle) -> Result<void>
	{
		auto *target {
			Arch::HandleManager::the().resolve<Arch::Thread>(
			    thread->process, thread_handle, Arch::HandleKind::Thread),
		};
		if (!target)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		if (!Arch::Scheduler::the().start_thread(target))
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		Debug::print_formatted("[Syscall] thread_start tid=%d pid=%d\n",
		    static_cast<int>(target->tid.id),
		    target->process ? static_cast<int>(target->process->pid.id) : -1);
		Debug::drain_logs();

		return Result<void>::Ok();
	}
};

}
