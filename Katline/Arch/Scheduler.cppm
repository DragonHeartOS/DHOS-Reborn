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
		void on_timer_tick();
		void yield();
		void block_current();
		void unblock(Thread *thread);
		[[noreturn]] void thread_exit();

		static auto the() -> Scheduler &;

	private:
		Thread *m_current_thread {};
		bool m_in_schedule {};

		CL::LinkedList<Process *> m_processes;
		CL::ArrayList<Thread *> m_ready_threads;
		CL::ArrayList<Thread *> m_zombies;
		usize m_ready_head {};

		u64 m_tid_counter {};
		u32 m_default_slice_ticks { 8 };

		Thread *m_idle_thread;

		auto is_in_ready_queue(Thread *thread) const -> bool;
		void enqueue_ready(Thread *thread);
		auto dequeue_ready() -> Thread *;
		void compact_ready_queue_if_needed();
		void reap_zombies();
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
	Debug::drain_logs();
	k_process = new Process {
		.parent = nullptr,
		.pid = { 0 },
		.threads = {},
		.cr3 = current_cr3(),
	};
	m_processes.push(k_process);
	m_idle_thread = make_thread(k_process, reinterpret_cast<uptr>(+[]() {
		asm volatile("cli");
		for (;;) {
			asm volatile("sti");
			asm volatile("hlt");
		}
	}),
	    false);
	Debug::print_formatted("[Scheduler] Scheduler initialized.\n");
	Debug::drain_logs();
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
	th->context.rsp = current_rsp();
	th->context.rip = reinterpret_cast<uptr>(__builtin_return_address(0));
	th->context.cs = current_cs();
	th->context.ss = current_ss();
	th->context.ds = current_ss();
	th->context.es = current_ss();
	th->context.fs = current_ss();
	th->context.gs = current_ss();
	th->context.rflags = 0x202;
	th->slice_remaining = m_default_slice_ticks;
	th->stack_is_heap_allocated = false;

	process->threads.push(th);
	m_current_thread = th;

	Debug::print_formatted(
	    "[Scheduler] Adopted current thread ID %d.\n", th->tid.id);
	Debug::drain_logs();
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
	th->slice_remaining = m_default_slice_ticks;
	th->stack_is_heap_allocated = true;

	process->threads.push(th);
	if (add_to_list)
		enqueue_ready(th);

	Debug::print_formatted("[Scheduler] Created thread for process ID %d with "
	                       "thread ID %d rip=0x%016x entry=0x%016x\n",
	    process->pid.id, th->tid.id, th->context.rip, th->entry_point);
	Debug::drain_logs();

	return th;
}

void Scheduler::schedule()
{
	if (m_in_schedule)
		return;
	m_in_schedule = true;

	auto *prev { m_current_thread };
	if (prev && prev->tstate == ThreadState::Running && prev != m_idle_thread) {
		if (m_ready_head >= m_ready_threads.size()) {
			m_in_schedule = false;
			return;
		}

		prev->tstate = ThreadState::Ready;
		enqueue_ready(prev);
	}

	auto *next { pick_next_thread() };
	if (!next)
		next = m_idle_thread;

	if (m_current_thread == next) {
		m_in_schedule = false;
		return;
	}

	if (next->process && next->process->cr3 != current_cr3())
		load_cr3(next->process->cr3);

	// Debug::print_formatted("[Scheduler] switch prev=%d next=%d "
	//                        "next_rip=0x%016x next_entry=0x%016x\n",
	//     prev ? static_cast<int>(prev->tid.id) : -1,
	//     static_cast<int>(next->tid.id), next->context.rip,
	//     next->entry_point);
	// Debug::drain_logs();

	next->tstate = ThreadState::Running;
	next->slice_remaining = m_default_slice_ticks;
	m_current_thread = next;

	m_in_schedule = false;
	CPUContext::switch_to(prev ? &prev->context : nullptr, &next->context);
}

auto Scheduler::current_thread() -> Thread * { return m_current_thread; }

void Scheduler::on_timer_tick()
{
	reap_zombies();

	auto *current { m_current_thread };
	if (!current || current == m_idle_thread
	    || current->tstate != ThreadState::Running)
		return;

	if (current->slice_remaining > 0)
		--current->slice_remaining;

	if (current->slice_remaining == 0) {
		current->slice_remaining = m_default_slice_ticks;
		schedule();
	}
}

void Scheduler::yield() { schedule(); }

void Scheduler::block_current()
{
	auto *current { m_current_thread };
	if (current)
		current->tstate = ThreadState::Blocked;
	schedule();
}

void Scheduler::unblock(Thread *thread)
{
	if (!thread || thread->tstate != ThreadState::Blocked)
		return;

	thread->tstate = ThreadState::Ready;
	enqueue_ready(thread);
}

[[noreturn]] void Scheduler::thread_exit()
{
	auto *current { m_current_thread };
	if (current == m_idle_thread) {
		for (;;)
			asm volatile("hlt");
	}
	if (current) {
		current->tstate = ThreadState::Dead;
		m_zombies.push(current);
	}
	m_current_thread = nullptr;
	schedule();
	for (;;)
		asm volatile("hlt");
}

auto Scheduler::pick_next_thread() -> Thread * { return dequeue_ready(); }

auto Scheduler::is_in_ready_queue(Thread *thread) const -> bool
{
	if (!thread)
		return false;

	for (usize i { m_ready_head }; i < m_ready_threads.size(); ++i) {
		if (m_ready_threads[i] == thread)
			return true;
	}
	return false;
}

void Scheduler::enqueue_ready(Thread *thread)
{
	if (!thread || thread == m_idle_thread)
		return;
	if (thread->tstate != ThreadState::Ready)
		return;
	if (is_in_ready_queue(thread))
		return;
	m_ready_threads.push(thread);
}

auto Scheduler::dequeue_ready() -> Thread *
{
	while (m_ready_head < m_ready_threads.size()) {
		auto *candidate { m_ready_threads[m_ready_head++] };
		if (!candidate)
			continue;
		if (candidate == m_idle_thread)
			continue;
		if (candidate->tstate != ThreadState::Ready)
			continue;
		compact_ready_queue_if_needed();
		return candidate;
	}

	compact_ready_queue_if_needed();
	return nullptr;
}

void Scheduler::compact_ready_queue_if_needed()
{
	if (!m_ready_head)
		return;
	if (m_ready_head < 64 && m_ready_head * 2 < m_ready_threads.size())
		return;

	auto remaining { m_ready_threads.size() - m_ready_head };
	for (usize i {}; i < remaining; ++i)
		m_ready_threads[i] = m_ready_threads[m_ready_head + i];
	while (m_ready_threads.size() > remaining)
		m_ready_threads.pop();
	m_ready_head = 0;
}

void Scheduler::reap_zombies()
{
	if (m_zombies.is_empty())
		return;

	for (usize i {}; i < m_zombies.size(); ++i) {
		auto *thread { m_zombies[i] };
		if (!thread)
			continue;
		if (thread->stack_is_heap_allocated && thread->kernel_stack_base) {
			auto *stack {
				reinterpret_cast<u8 *>(thread->kernel_stack_base),
			};
			delete[] stack;
		}
		delete thread;
	}
	m_zombies.clear();
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
