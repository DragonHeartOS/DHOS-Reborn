export module Katline:SyscallGetProcessInfo;

import CommonLib;
import KatlineAPI;
import :HandleManager;
import :Scheduler;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::GetProcessInfo> {
	static constexpr bool requires_current_thread = true;

	static auto call(Arch::Thread *thread, Handle process,
	    UserPtr<ProcessInfo> process_info, UserPtr<u64> buffer_size)
	    -> Result<void>
	{
		auto const actual_process {
			Arch::HandleManager::the().resolve<Arch::Process>(
			    thread->process, process, Arch::HandleKind::Process),
		};

		if (process_info.is_null()) {
			return copy_out(
			    buffer_size, sizeof(ProcessInfo) + actual_process->name.size());
		} else {
			auto const *info = new ProcessInfo {
				.pid = actual_process->pid,
				.handle_count = actual_process->handle_count,
				.name_len = actual_process->name.size(),
			};
			return copy_out(process_info, info,
			    sizeof(ProcessInfo) + actual_process->name.size());
		}
	}
};

}
