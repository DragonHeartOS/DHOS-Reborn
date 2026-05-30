export module Katline:Thread;

import CommonLib;
import :CPU;

export {
	namespace Katline::Arch {

	struct ProcessID {
		u64 id;
	};

	struct ThreadID {
		u64 id;
	};

	enum class ThreadState : u8 {
		Ready,
		Running,
		Blocked,
		Dead,
	};

	enum class ProcessKind : u8 {
		Kernel,
		Driver,
		User,
	};

	struct Thread;

	struct Process {
		Process *parent {};

		ProcessID pid {};
		CL::ArrayList<Thread *> threads {};

		u64 cr3;
	};

	struct Thread {
		ThreadID tid {};
		ThreadState tstate { ThreadState::Ready };
		uptr entry_point {};
		CPUContext context {};
		u32 slice_remaining {};

		uptr kernel_stack_base {};
		usize kernel_stack_size {};
		bool stack_is_heap_allocated {};
		Process *process {};

		u64 fs_base {};
		u64 gs_base {};
	};

	}
};
