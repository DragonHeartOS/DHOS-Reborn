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

	static auto call(Arch::Thread *thread,
	    UserPtr<ProcessMemoryMapOptions> options_ptr) -> Result<void>
	{
		if (options_ptr.is_null())
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		auto *options { options_ptr.get() };

		auto *process {
			Arch::HandleManager::the().resolve<Arch::Process>(
			    thread->process, options->process, Arch::HandleKind::Process),
		};

		auto *object {
			Arch::HandleManager::the().resolve<Memory::MemoryObject>(
			    thread->process, options->memory_object,
			    Arch::HandleKind::MemoryObject),
		};

		if (!process || !object)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		auto const mmio_res { require_mmio_mapping_capability(object) };
		if (mmio_res.is_err())
			return Result<void>::Err(mmio_res.unwrap_err());

		UserPtr<void *> out_addr { &options->addr };

		auto const map_res {
			map_memory_object(process, object, options->offset, options->size,
			    options->flags, out_addr),
		};
		if (map_res.is_err())
			return Result<void>::Err(map_res.unwrap_err());

		return Result<void>::Ok();
	}
};

}
