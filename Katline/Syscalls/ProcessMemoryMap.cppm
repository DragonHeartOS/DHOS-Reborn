export module Katline:SyscallProcessMemoryMap;

import CommonLib;
import KatlineAPI;
import :HandleManager;
import :MemoryObject;
import :Scheduler;
import :SyscallKernelContract;
import :SyscallMemoryMap;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ProcessMemoryMap> {
	static constexpr bool requires_current_thread = true;
	static constexpr ProcessCapabilityFlags required_capabilities {
		ProcessCapability::ManageProcesses
	};

	static auto call(Arch::Thread *thread, Handle process_handle,
	    Handle memory_object, u64 offset, u64 size,
	    Katline::MemoryMapFlags flags, UserPtr<void *> out_addr) -> Result<void>
	{
		auto *process {
			Arch::HandleManager::the().resolve<Arch::Process>(
			    thread->process, process_handle, Arch::HandleKind::Process),
		};
		auto *object {
			Arch::HandleManager::the().resolve<Memory::MemoryObject>(
			    thread->process, memory_object, Arch::HandleKind::MemoryObject),
		};
		if (!process || !object)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		if (object->kind == Memory::MemoryObjectKind::MMIO) {
			if (auto const res {
			        require_capabilities({ ProcessCapability::MapMMIO }),
			    };
			    res.is_err())
				return Result<void>::Err(res.unwrap_err());
		}

		return map_memory_object(
		    process, object, offset, size, flags, out_addr);
	}
};

}
