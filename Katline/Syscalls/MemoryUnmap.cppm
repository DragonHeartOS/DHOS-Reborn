export module Katline:SyscallMemoryUnmap;

import CommonLib;
import KatlineAPI;
import :Scheduler;
import :SyscallMemoryMapHelpers;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::MemoryUnmap> {
	static auto call(UserPtr<void> addr, u64 size) -> Result<void>
	{
		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		return unmap_memory_object(thread->process, addr.addr(), size);
	}
};

}
