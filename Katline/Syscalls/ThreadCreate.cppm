export module Katline:SyscallThreadCreate;

import CommonLib;
import :Debug;
import :HandleManager;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::ThreadCreate> {
	static constexpr bool requires_current_thread = true;

	static auto call(Arch::Thread *thread, Handle process_handle, u64 entry,
	    u64 stack) -> Result<Handle>
	{
		Debug::print_formatted("[Syscall] thread_create call handle=%d "
		                       "entry=0x%016x stack=0x%016x\n",
		    static_cast<int>(process_handle.id), entry, stack);
		Debug::drain_logs();

		auto *process {
			Arch::HandleManager::the().resolve<Arch::Process>(
			    thread->process, process_handle, Arch::HandleKind::Process),
		};
		if (!process || !entry || !stack
		    || !is_valid_user_address(static_cast<uptr>(entry))
		    || !is_valid_user_address(static_cast<uptr>(stack))) {
			Debug::print_formatted("[Syscall] thread_create invalid process=%d "
			                       "entry=0x%016x stack=0x%016x\n",
			    process ? static_cast<int>(process->pid.id) : -1, entry, stack);
			Debug::drain_logs();
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});
		}

		auto *created {
			Arch::Scheduler::the().make_user_thread(
			    process, static_cast<uptr>(entry), static_cast<uptr>(stack)),
		};
		if (!created) {
			Debug::print_formatted(
			    "[Syscall] thread_create make_user_thread failed pid=%d "
			    "entry=0x%016x stack=0x%016x\n",
			    static_cast<int>(process->pid.id), entry, stack);
			Debug::drain_logs();
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});
		}

		Debug::print_formatted("[Syscall] thread_create pid=%d entry=0x%016x "
		                       "stack=0x%016x tid=%d\n",
		    static_cast<int>(process->pid.id), entry, stack,
		    static_cast<int>(created->tid.id));
		Debug::drain_logs();

		auto handle {
			Arch::HandleManager::the().open(
			    thread->process, Arch::HandleKind::Thread, created),
		};
		if (handle.is_invalid()) {
			Debug::print_formatted(
			    "[Syscall] thread_create open handle failed pid=%d tid=%d\n",
			    static_cast<int>(process->pid.id),
			    static_cast<int>(created->tid.id));
			Debug::drain_logs();
			return Result<Handle>::Err(ErrorsV::InvalidArgument {});
		}

		return Result<Handle>::Ok(handle);
	}
};

}
