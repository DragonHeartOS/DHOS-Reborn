export module Katline:SyscallIPCSend;

import CommonLib;
import KatlineAPI;
import :Scheduler;
import :SyscallKernelContract;

namespace Katline::Syscalls {

static CL::Atomic<u64> g_id_counter {};

template<> struct Spec<SyscallNumber::IPCSend> {
	static auto call(u64 endpoint, UserPtrConst<IPC::Message> message,
	    u64 flags) -> Result<void>
	{
		auto &sched { Arch::Scheduler::the() };
		auto *thread { sched.current_thread() };
		if (!thread || !thread->process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		CL::ignore_unused(flags);
		// We need the process list from the scheduler, that's why it's there
		// and not here.
		return copy_in(message).and_then([&](IPC::Message msg) -> Result<void> {
			msg.sender = thread->process->endpoint_id;
			msg.id = g_id_counter.fetch_add(1);

			if (!sched.send_ipc_message(endpoint, msg))
				return Result<void>::Err(ErrorsV::InvalidArgument {});

			return Result<void>::Ok();
		});
	}
};

}
