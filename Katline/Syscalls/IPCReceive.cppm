export module Katline:SyscallIPCReceive;

import CommonLib;
import KatlineAPI;
import :Scheduler;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::IPCReceive> {
	static constexpr bool requires_current_thread = true;

	static auto call(Arch::Thread *thread, UserPtr<IPC::Message> out_message,
	    u64 flags) -> Result<void>
	{
		CL::ignore_unused(flags);

		return thread->process->ipc_message_queue.try_pop()
		    .ok_or(ErrorsV::EmptyQueue {})
		    .and_then([&](IPC::Message msg) -> Result<void> {
			    return copy_out(out_message, msg);
		    });
	}
};

}
