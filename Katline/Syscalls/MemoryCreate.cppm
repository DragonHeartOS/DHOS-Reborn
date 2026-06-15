export module Katline:SyscallMemoryCreate;

import CommonLib;
import KatlineAPI;
import :HandleManager;
import :MemoryObject;
import :Scheduler;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::MemoryCreate> {
	static constexpr bool requires_current_thread = true;

	static auto call(Arch::Thread *thread, u64 size) -> Result<Handle>
	{
		if (size == 0)
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		auto *object {
			Memory::MemoryObject::create_allocated(static_cast<usize>(size)),
		};
		if (!object)
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});

		auto const handle {
			Arch::HandleManager::the().open(
			    thread->process, Arch::HandleKind::MemoryObject, object),
		};
		if (handle.is_invalid()) {
			Memory::release_memory_object(object);
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});
		}

		return Result<Handle>::Ok(handle);
	}
};

}
