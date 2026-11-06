module;

#include <string.h>

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
			auto const total_size {
				sizeof(ProcessInfo) + actual_process->name.size(),
			};
			if (total_size > 512)
				return Result<void>::Err(ErrorsV::InvalidArgument {});
			alignas(ProcessInfo) char storage[512] {};
			auto *info { reinterpret_cast<ProcessInfo *>(storage) };
			info->pid = actual_process->pid;
			info->endpoint_id = actual_process->endpoint_id;
			info->handle_count = actual_process->handle_count;
			info->name_len = actual_process->name.size();
			memcpy(const_cast<char *>(info->name), actual_process->name.data(),
			    actual_process->name.size());
			return copy_out(process_info, info, total_size);
		}
	}
};

}
