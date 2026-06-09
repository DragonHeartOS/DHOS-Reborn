export module Katline:SyscallMemoryMap;

import CommonLib;
import KatlineAPI;
import :HandleManager;
import :MemoryObject;
import :Scheduler;
import :SyscallMemoryMapHelpers;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::MemoryMap> {
	static auto call(Handle memory_object, u64 offset, u64 size,
	    Katline::MemoryMapFlags flags, UserPtr<void *> out_addr) -> Result<void>
	{
		auto *thread { Arch::Scheduler::the().current_thread() };
		if (!thread || !thread->process)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		auto *object {
			Arch::HandleManager::the().resolve<Memory::MemoryObject>(
			    thread->process, memory_object, Arch::HandleKind::MemoryObject),
		};
		if (!object)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		if (object->kind == Memory::MemoryObjectKind::MMIO) {
			if (auto const res {
			        require_capability(ProcessCapability::MapMMIO),
			    };
			    res.is_err())
				return Result<void>::Err(res.unwrap_err());
		}

		return map_memory_object(
		    thread->process, object, offset, size, flags, out_addr);
	}
};

}
