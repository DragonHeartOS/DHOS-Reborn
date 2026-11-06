export module Katline:SyscallProcessMemoryMap;

import CommonLib;
import :Debug;
import KatlineAPI;
import :HandleManager;
import :MemoryObject;
import :Scheduler;
import :SyscallKernelContract;
import :SyscallMemoryMap;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ProcessMemoryMap> {
	static constexpr bool requires_current_thread = true;

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
		Debug::print_formatted(
		    "[Syscall] process_memory_map pid=%d handle=%d offset=0x%016x "
		    "size=0x%016x addr=0x%016x flags=0x%016x\n",
		    static_cast<int>(process->pid.id),
		    static_cast<int>(options->memory_object.id), options->offset,
		    options->size, reinterpret_cast<u64>(options->addr),
		    options->flags.raw());
		Debug::drain_logs();

		auto const map_res {
			map_memory_object(process, object, options->offset, options->size,
			    options->flags, out_addr,
			    reinterpret_cast<uptr>(options->addr)),
		};
		if (map_res.is_err()) {
			Debug::print_formatted(
			    "[Syscall] process_memory_map failed pid=%d\n",
			    static_cast<int>(process->pid.id));
			Debug::drain_logs();
			return Result<void>::Err(map_res.unwrap_err());
		}

		Debug::print_formatted(
		    "[Syscall] process_memory_map ok pid=%d mapped=0x%016x\n",
		    static_cast<int>(process->pid.id),
		    reinterpret_cast<u64>(options->addr));
		Debug::drain_logs();

		return Result<void>::Ok();
	}
};

}
