export module Katline:Scheduler;

import CommonLib;
import :CPU;
import :Thread;
import :Debug;

export {
	namespace Katline::Arch {

	struct Scheduler {
		void init();
		auto adopt_current_thread(
		    Process *process, uptr stack_base, usize stack_size) -> Thread *;
		auto make_thread(Process *process, uptr function_ptr,
		    bool add_to_list = true) -> Thread *;
		void schedule();
		auto pick_next_thread() -> Thread *;
		auto current_thread() -> Thread *;
		void yield();
		[[noreturn]] void thread_exit();

		static auto the() -> Scheduler &;

	private:
		Thread *m_current_thread {};

		CL::LinkedList<Process *> m_processes;
		CL::LinkedList<Thread *> m_threads;

		u64 m_tid_counter {};
		u64 m_last_scheduled_tid {};

		Thread *m_idle_thread;
	};

	extern Process *k_process;

	}
}

namespace Katline::Arch {
[[noreturn]] void thread_entry_trampoline();
}

namespace Katline::Arch {

constexpr auto KERNEL_STACK_SIZE { 16'384 };
Process *k_process {};

void Scheduler::init()
{
	Debug::print_formatted("[Scheduler] Initializing...\n");
	k_process = new Process {
		.parent = nullptr,
		.pid = { 0 },
		.threads = {},
		.cr3 = current_cr3(),
	};
	m_idle_thread = make_thread(k_process, reinterpret_cast<uptr>(+[]() {
		asm volatile("cli");
		for (;;) {
			asm volatile("sti");
			asm volatile("hlt");
		}
	}),
	    false);
	Debug::print_formatted("[Scheduler] Scheduler initialized.\n");
}

auto Scheduler::the() -> Scheduler &
{
	static Scheduler instance {};
	return instance;
}

auto Scheduler::adopt_current_thread(
    Process *process, uptr stack_base, usize stack_size) -> Thread *
{
	auto *th { new Thread {} };

	th->tid = { ++m_tid_counter };
	th->tstate = ThreadState::Running;
	th->process = process;
	th->kernel_stack_base = stack_base;
	th->kernel_stack_size = stack_size;
	th->context = CPUContext {};
	th->context.save_current();
	th->context.rsp = current_rsp();
	th->context.cs = current_cs();
	th->context.ss = current_ss();
	th->context.ds = current_ss();
	th->context.es = current_ss();
	th->context.fs = current_ss();
	th->context.gs = current_ss();
	th->context.rflags = 0x202;

	process->threads.push(th);
	m_threads.push(th);
	m_current_thread = th;
	m_last_scheduled_tid = th->tid.id;

	Debug::print_formatted(
	    "[Scheduler] Adopted current thread ID %d.\n", th->tid.id);
	return th;
}

auto Scheduler::make_thread(
    Process *process, uptr function_ptr, bool add_to_list) -> Thread *
{
	auto *th { new Thread {} };

	th->tid = { ++m_tid_counter };
	th->tstate = ThreadState::Ready;
	th->entry_point = function_ptr;
	th->process = process;
	th->kernel_stack_base = reinterpret_cast<uptr>(new u8[KERNEL_STACK_SIZE]);
	th->kernel_stack_size = KERNEL_STACK_SIZE;
	th->context = CPUContext {};
	th->context.rsp = (th->kernel_stack_base + th->kernel_stack_size) & ~0xFull;
	th->context.rip = reinterpret_cast<uptr>(&thread_entry_trampoline);
	th->context.rflags = 0x202;
	th->context.cs = current_cs();
	th->context.ss = current_ss();
	th->context.ds = current_ss();
	th->context.es = current_ss();
	th->context.fs = current_ss();
	th->context.gs = current_ss();

	process->threads.push(th);
	if (add_to_list)
		m_threads.push(th);

	Debug::print_formatted(
	    "[Scheduler] Created thread for process ID %d with thread ID %d.\n",
	    process->pid.id, th->tid.id);

	return th;
}

void Scheduler::schedule()
{
	auto *prev { m_current_thread };
	auto *next { pick_next_thread() };
	if (!next)
		return;

	if (m_current_thread == next)
		return;

	if (prev && prev->tstate == ThreadState::Running)
		prev->tstate = ThreadState::Ready;

	if (next->process && next->process->cr3 != current_cr3())
		load_cr3(next->process->cr3);

	next->tstate = ThreadState::Running;
	m_current_thread = next;
	m_last_scheduled_tid = next->tid.id;

	CPUContext::switch_to(prev ? &prev->context : nullptr, &next->context);
}

auto Scheduler::current_thread() -> Thread * { return m_current_thread; }

void Scheduler::yield() { schedule(); }

[[noreturn]] void Scheduler::thread_exit()
{
	auto *current { m_current_thread };
	if (current == m_idle_thread) {
		for (;;)
			asm volatile("hlt");
	}
	if (current)
		current->tstate = ThreadState::Dead;
	m_current_thread = nullptr;
	schedule();
	for (;;)
		asm volatile("hlt");
}

auto Scheduler::pick_next_thread() -> Thread *
{
	Thread *best_after_cursor {};
	Thread *best_wrap {};

	m_threads.iter().for_each([&](Thread *thread) {
		if (thread->tstate != ThreadState::Ready)
			return;

		if (!best_wrap || thread->tid.id < best_wrap->tid.id)
			best_wrap = thread;

		if (thread->tid.id > m_last_scheduled_tid) {
			if (!best_after_cursor
			    || thread->tid.id < best_after_cursor->tid.id)
				best_after_cursor = thread;
		}
	});

	if (best_after_cursor)
		return best_after_cursor;

	if (best_wrap)
		return best_wrap;

	if (m_current_thread && m_current_thread->tstate == ThreadState::Running)
		return m_current_thread;

	return m_idle_thread;
}

[[noreturn]] void thread_entry_trampoline()
{
	auto *thread { Scheduler::the().current_thread() };
	if (!thread)
		Scheduler::the().thread_exit();

	auto entry { reinterpret_cast<void (*)()>(thread->entry_point) };
	entry();
	Scheduler::the().thread_exit();
}

}
