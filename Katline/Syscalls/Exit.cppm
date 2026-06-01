export module Katline:SyscallExit;

import CommonLib;
import :Debug;
import :Scheduler;
import KatlineAPI;
import :SyscallKernelContract;

export {
	namespace Katline::Syscalls {
	[[noreturn]] auto exit(i32 code) -> void;
	}
}

namespace Katline::Syscalls {

[[noreturn]] auto exit(i32 code) -> void
{
	auto *thread { Arch::Scheduler::the().current_thread() };
	Debug::print_formatted("[Syscall] exit tid=%d code=%d\n",
	    thread ? static_cast<int>(thread->tid.id) : -1, code);
	Arch::Scheduler::the().thread_exit();
}

template<> struct Spec<SyscallNumber::Exit> {
	[[noreturn]] static auto call(i32 code) -> Result<void> { exit(code); }
};

}
