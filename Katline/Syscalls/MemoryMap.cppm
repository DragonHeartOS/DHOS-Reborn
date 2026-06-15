export module Katline:SyscallMemoryMap;

import CommonLib;
import KatlineAPI;
import :HandleManager;
import :MemoryObject;
import :Scheduler;
import :SyscallMemoryMapHelpers;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::MemoryMap> {
	static constexpr bool requires_current_thread = true;

	static auto call(Arch::Thread *thread, Handle memory_object, u64 offset,
	    u64 size, Katline::MemoryMapFlags flags, UserPtr<void *> out_addr)
	    -> Result<u64>
	{
		auto *object {
			Arch::HandleManager::the().resolve<Memory::MemoryObject>(
			    thread->process, memory_object, Arch::HandleKind::MemoryObject),
		};
		if (!object) {
			return Result<u64>::Err(ErrorsV::InvalidArgument {});
		}

		auto const mmio_res { require_mmio_mapping_capability(object) };
		if (mmio_res.is_err()) {
			return Result<u64>::Err(mmio_res.unwrap_err());
		}

		return map_memory_object(
		    thread->process, object, offset, size, flags, out_addr);
	}
};

}
