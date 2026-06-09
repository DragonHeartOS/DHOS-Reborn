export module Katline:SyscallProcessMemoryUnmap;

import CommonLib;
import KatlineAPI;
import :HandleManager;
import :Scheduler;
import :SyscallKernelContract;
import :SyscallMemoryMap;

namespace Katline::Syscalls {
template<> struct Spec<SyscallNumber::ProcessMemoryUnmap> {
	static auto call(Handle process_handle, UserPtr<void> addr, u64 size)
	    -> Result<void>
	{
		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		auto *process {
			Arch::HandleManager::the().resolve<Arch::Process>(
			    thread->process, process_handle, Arch::HandleKind::Process),
		};
		if (!process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		return unmap_memory_object(process, addr.addr(), size);
	}
};

}
