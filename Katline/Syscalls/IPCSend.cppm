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

			// TODO: Should probably validate the handles before actually send
			// ing the IPC message. Since there's currently no use of handles,
			// there's no way to way to do this.
			auto const status { sched.send_ipc_message(endpoint, msg) };
			if (status == Arch::IPCSendStatus::InvalidEndpoint)
				return Result<void>::Err(ErrorsV::InvalidArgument {});
			if (status == Arch::IPCSendStatus::QueueFull)
				return Result<void>::Err(ErrorsV::QueueFull {});

			return Result<void>::Ok();
		});
	}
};

}
