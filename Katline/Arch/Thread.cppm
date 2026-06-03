export module Katline:Thread;

import CommonLib;
import KatlineAPI;
import :CPU;

export {
	namespace Katline::Arch {

	enum class ThreadState : u8 {
		Created,
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

	enum class ProcessState : u8 {
		Running,
		Dead,
	};

	struct Thread;

	struct Process {
		Process *parent {};

		ProcessID pid {};
		CL::String name {};
		u64 handle_count {};
		ProcessState state { ProcessState::Running };
		CL::ArrayList<Thread *> threads {};
		CL::MpscQueue<IPC::Message, 256> ipc_message_queue {};
		u64 endpoint_id {};

		u64 cr3;
	};

	struct Thread {
		ThreadID tid {};
		ThreadState tstate { ThreadState::Ready };
		u64 handle_count {};
		uptr entry_point {};
		CPUContext context {};
		u32 slice_remaining {};
		u32 home_lapic_id {};
		u32 last_lapic_id {};

		uptr kernel_stack_base {};
		usize kernel_stack_size {};
		bool stack_is_heap_allocated {};
		Process *process {};

		u64 fs_base {};
		u64 gs_base {};
	};

	}
};
