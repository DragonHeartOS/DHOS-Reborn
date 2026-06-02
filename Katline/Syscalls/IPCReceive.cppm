export module Katline:SyscallIPCReceive;

import CommonLib;
import KatlineAPI;
import :Scheduler;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::IPCReceive> {
	static auto call(UserPtr<IPC::Message> out_message, u64 flags)
	    -> Result<void>
	{
		CL::ignore_unused(flags);

		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		return thread->process->ipc_message_queue.try_pop()
		    .ok_or(ErrorsV::EmptyQueue {})
		    .and_then([&](IPC::Message msg) -> Result<void> {
			    return copy_out(out_message, msg);
		    });
	}
};

}
