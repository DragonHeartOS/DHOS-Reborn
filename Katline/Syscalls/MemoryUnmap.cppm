export module Katline:SyscallMemoryUnmap;

import CommonLib;
import KatlineAPI;
import :Scheduler;
import :SyscallMemoryMapHelpers;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::MemoryUnmap> {
	static constexpr bool requires_current_thread = true;

	static auto call(Arch::Thread *thread, UserPtr<void> addr, u64 size)
	    -> Result<void>
	{
		return unmap_memory_object(thread->process, addr.addr(), size);
	}
};

}
