export module Katline:Scheduler;

import CommonLib;
import :CPU;
import :Thread;
import :Debug;
import :GDT;
import :Sync;

export {
	namespace Katline::Arch {

	struct Scheduler {
		void init();
		void init_current_cpu();
		auto create_process(Process *parent, u64 cr3) -> Process *;
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
		struct CPUState {
			u32 lapic_id {};
			Thread *current_thread {};
			Thread *idle_thread {};
			bool in_schedule {};
			bool online {};

			CL::ArrayList<Thread *> ready_threads;
			usize ready_head {};
			Sync::SpinLock queue_lock;
		};

		CL::LinkedList<Process *> m_processes;
		CL::ArrayList<Thread *> m_zombies;
		CL::ArrayList<CPUState *> m_cpus;
		Sync::SpinLock m_global_lock;
		bool m_global_initialized {};

		u64 m_pid_counter {};
		u64 m_tid_counter {};
		u32 m_default_slice_ticks { 8 };

		auto local_lapic_id() const -> u32;
		auto find_cpu_state(u32 lapic_id) -> CPUState *;
		auto get_or_create_cpu_state(u32 lapic_id) -> CPUState *;
		auto current_cpu_state() -> CPUState *;
		auto is_in_ready_queue(CPUState const &cpu, Thread *thread) const
		    -> bool;
		void enqueue_ready(CPUState &cpu, Thread *thread);
		auto dequeue_ready(CPUState &cpu) -> Thread *;
		void compact_ready_queue_if_needed(CPUState &cpu);
		auto steal_thread_for_cpu(CPUState &cpu) -> Thread *;
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
	{
		Sync::ScopedIrqSpinLock guard { m_global_lock };
		if (!m_global_initialized) {
			Debug::print_formatted("[Scheduler] Initializing...\n");
			Debug::drain_logs();
			k_process = new Process {
				.parent = nullptr,
				.pid = { 0 },
				.threads = {},
				.cr3 = current_cr3(),
			};
			m_processes.push(k_process);
			m_global_initialized = true;
		}
	}

	init_current_cpu();
	Debug::print_formatted("[Scheduler] Scheduler initialized on CPU %d.\n",
	    static_cast<int>(local_lapic_id()));
	Debug::drain_logs();
}

auto Scheduler::the() -> Scheduler &
{
	static Scheduler instance {};
	return instance;
}

auto Scheduler::create_process(Process *parent, u64 cr3) -> Process *
{
	auto *process { new Process {} };
	{
		Sync::ScopedIrqSpinLock guard { m_global_lock };
		process->parent = parent;
		process->pid = { ++m_pid_counter };
		process->cr3 = cr3;
		m_processes.push(process);
	}
	return process;
}

void Scheduler::init_current_cpu()
{
	auto lapic_id { local_lapic_id() };
	auto *cpu { get_or_create_cpu_state(lapic_id) };
	if (!cpu)
		return;

	{
		Sync::ScopedIrqSpinLock guard { cpu->queue_lock };
		cpu->online = true;
	}

	if (!cpu->idle_thread) {
		auto *idle {
			make_thread(k_process, reinterpret_cast<uptr>(+[]() {
			    for (;;) {
				    asm volatile("sti");
				    asm volatile("hlt");
			    }
			}),
			    false),
		};
		if (idle)
			cpu->idle_thread = idle;
	}

	Debug::print_formatted(
	    "[Scheduler] CPU %d online.\n", static_cast<int>(lapic_id));
	Debug::drain_logs();
}

auto Scheduler::adopt_current_thread(
    Process *process, uptr stack_base, usize stack_size) -> Thread *
{
	auto *cpu { current_cpu_state() };
	if (!cpu)
		return nullptr;

	auto *th { new Thread {} };

	{
		Sync::ScopedIrqSpinLock guard { m_global_lock };
		th->tid = { ++m_tid_counter };
	}
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
	th->home_lapic_id = cpu->lapic_id;
	th->last_lapic_id = cpu->lapic_id;

	{
		Sync::ScopedIrqSpinLock guard { m_global_lock };
		process->threads.push(th);
	}
	{
		Sync::ScopedIrqSpinLock guard { cpu->queue_lock };
		cpu->current_thread = th;
	}

	Debug::print_formatted("[Scheduler] CPU %d adopted thread ID %d.\n",
	    static_cast<int>(cpu->lapic_id), th->tid.id);
	Debug::drain_logs();
	return th;
}

auto Scheduler::make_thread(
    Process *process, uptr function_ptr, bool add_to_list) -> Thread *
{
	auto *cpu { current_cpu_state() };
	if (!cpu)
		return nullptr;

	auto *th { new Thread {} };

	{
		Sync::ScopedIrqSpinLock guard { m_global_lock };
		th->tid = { ++m_tid_counter };
	}

	th->tstate = ThreadState::Ready;
	th->entry_point = function_ptr;
	th->process = process;

	th->kernel_stack_base = reinterpret_cast<uptr>(new u8[KERNEL_STACK_SIZE]);
	th->kernel_stack_size = KERNEL_STACK_SIZE;

	auto stack_top {
		(th->kernel_stack_base + th->kernel_stack_size) & ~0xFull,
	};

	stack_top -= 8;
	*reinterpret_cast<u64 *>(stack_top) = 0;

	th->context = CPUContext {};
	th->context.rsp = stack_top;
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
	th->home_lapic_id = cpu->lapic_id;
	th->last_lapic_id = cpu->lapic_id;

	{
		Sync::ScopedIrqSpinLock guard { m_global_lock };
		process->threads.push(th);
	}

	if (add_to_list) {
		Sync::ScopedIrqSpinLock guard { cpu->queue_lock };
		enqueue_ready(*cpu, th);
	}

	Debug::print_formatted("[Scheduler] CPU %d created thread pid=%d tid=%d "
	                       "rip=0x%016x rsp=0x%016x entry=0x%016x\n",
	    static_cast<int>(cpu->lapic_id), process->pid.id, th->tid.id,
	    th->context.rip, th->context.rsp, th->entry_point);
	Debug::drain_logs();

	return th;
}

void Scheduler::schedule()
{
	auto *cpu { current_cpu_state() };
	if (!cpu)
		return;

	Thread *prev {};
	Thread *next {};

	{
		Sync::ScopedIrqSpinLock guard { cpu->queue_lock };
		if (cpu->in_schedule)
			return;
		cpu->in_schedule = true;

		prev = cpu->current_thread;
		if (prev && prev->tstate == ThreadState::Running
		    && prev != cpu->idle_thread) {
			prev->tstate = ThreadState::Ready;
			enqueue_ready(*cpu, prev);
		}

		next = dequeue_ready(*cpu);
	}

	if (!next)
		next = steal_thread_for_cpu(*cpu);

	{
		Sync::ScopedIrqSpinLock guard { cpu->queue_lock };
		if (!next)
			next = dequeue_ready(*cpu);
		if (!next)
			next = cpu->idle_thread;

		if (cpu->current_thread == next) {
			cpu->in_schedule = false;
			return;
		}

		prev = cpu->current_thread;
		next->tstate = ThreadState::Running;
		next->slice_remaining = m_default_slice_ticks;
		next->last_lapic_id = cpu->lapic_id;
		cpu->current_thread = next;
		cpu->in_schedule = false;
	}

	if (next->process && next->process->cr3 != current_cr3())
		load_cr3(next->process->cr3);

	set_kernel_stack_for_current_cpu(
	    next->kernel_stack_base + next->kernel_stack_size);
	CPUContext::switch_to(prev ? &prev->context : nullptr, &next->context);
}

auto Scheduler::pick_next_thread() -> Thread *
{
	auto *cpu { current_cpu_state() };
	if (!cpu)
		return nullptr;

	{
		Sync::ScopedIrqSpinLock guard { cpu->queue_lock };
		auto *local_next { dequeue_ready(*cpu) };
		if (local_next)
			return local_next;
	}

	return steal_thread_for_cpu(*cpu);
}

auto Scheduler::current_thread() -> Thread *
{
	auto *cpu { current_cpu_state() };
	if (!cpu)
		return nullptr;
	return cpu->current_thread;
}

void Scheduler::on_timer_tick()
{
	reap_zombies();

	auto *cpu { current_cpu_state() };
	if (!cpu)
		return;

	auto *current { cpu->current_thread };
	if (!current || current == cpu->idle_thread
	    || current->tstate != ThreadState::Running)
		return;

	if (current->slice_remaining > 0)
		--current->slice_remaining;

	if (current->slice_remaining == 0) {
		schedule();
	}
}

void Scheduler::yield() { schedule(); }

void Scheduler::block_current()
{
	auto *cpu { current_cpu_state() };
	if (!cpu)
		return;

	auto *current { cpu->current_thread };
	if (current)
		current->tstate = ThreadState::Blocked;
	schedule();
}

void Scheduler::unblock(Thread *thread)
{
	if (!thread || thread->tstate != ThreadState::Blocked)
		return;

	thread->tstate = ThreadState::Ready;

	CPUState *target_cpu {};
	{
		Sync::ScopedIrqSpinLock guard { m_global_lock };
		target_cpu = find_cpu_state(thread->home_lapic_id);
		if (!target_cpu || !target_cpu->online)
			target_cpu = find_cpu_state(local_lapic_id());
	}
	if (!target_cpu)
		return;

	Sync::ScopedIrqSpinLock queue_guard { target_cpu->queue_lock };
	enqueue_ready(*target_cpu, thread);
}

[[noreturn]] void Scheduler::thread_exit()
{
	auto *cpu { current_cpu_state() };
	if (!cpu) {
		for (;;)
			asm volatile("hlt");
	}

	auto *current { cpu->current_thread };
	if (current == cpu->idle_thread) {
		for (;;)
			asm volatile("hlt");
	}
	if (current) {
		current->tstate = ThreadState::Dead;
		Sync::ScopedIrqSpinLock global_guard { m_global_lock };
		m_zombies.push(current);
	}
	{
		Sync::ScopedIrqSpinLock queue_guard { cpu->queue_lock };
		cpu->current_thread = nullptr;
	}
	schedule();
	for (;;)
		asm volatile("hlt");
}

auto Scheduler::local_lapic_id() const -> u32 { return current_lapic_id(); }

auto Scheduler::find_cpu_state(u32 lapic_id) -> CPUState *
{
	for (usize i {}; i < m_cpus.size(); ++i) {
		auto *cpu { m_cpus[i] };
		if (cpu && cpu->lapic_id == lapic_id)
			return cpu;
	}
	return nullptr;
}

auto Scheduler::get_or_create_cpu_state(u32 lapic_id) -> CPUState *
{
	Sync::ScopedIrqSpinLock guard { m_global_lock };
	auto *existing { find_cpu_state(lapic_id) };
	if (existing)
		return existing;

	auto *created { new CPUState {} };
	created->lapic_id = lapic_id;
	m_cpus.push(created);
	return created;
}

auto Scheduler::current_cpu_state() -> CPUState *
{
	Sync::ScopedIrqSpinLock guard { m_global_lock };
	return find_cpu_state(local_lapic_id());
}

auto Scheduler::is_in_ready_queue(CPUState const &cpu, Thread *thread) const
    -> bool
{
	if (!thread)
		return false;

	for (usize i { cpu.ready_head }; i < cpu.ready_threads.size(); ++i) {
		if (cpu.ready_threads[i] == thread)
			return true;
	}
	return false;
}

void Scheduler::enqueue_ready(CPUState &cpu, Thread *thread)
{
	if (!thread || thread == cpu.idle_thread)
		return;
	if (thread->tstate != ThreadState::Ready)
		return;
	if (is_in_ready_queue(cpu, thread))
		return;
	cpu.ready_threads.push(thread);
}

auto Scheduler::dequeue_ready(CPUState &cpu) -> Thread *
{
	while (cpu.ready_head < cpu.ready_threads.size()) {
		auto *candidate { cpu.ready_threads[cpu.ready_head++] };
		if (!candidate)
			continue;
		if (candidate == cpu.idle_thread)
			continue;
		if (candidate->tstate != ThreadState::Ready)
			continue;
		compact_ready_queue_if_needed(cpu);
		return candidate;
	}

	compact_ready_queue_if_needed(cpu);
	return nullptr;
}

void Scheduler::compact_ready_queue_if_needed(CPUState &cpu)
{
	if (!cpu.ready_head)
		return;
	if (cpu.ready_head < 64 && cpu.ready_head * 2 < cpu.ready_threads.size())
		return;

	auto remaining { cpu.ready_threads.size() - cpu.ready_head };
	usize out {};
	for (usize i {}; i < remaining; ++i) {
		auto *candidate { cpu.ready_threads[cpu.ready_head + i] };
		if (candidate)
			cpu.ready_threads[out++] = candidate;
	}
	while (cpu.ready_threads.size() > out)
		cpu.ready_threads.pop();
	cpu.ready_head = 0;
}

auto Scheduler::steal_thread_for_cpu(CPUState &cpu) -> Thread *
{
	CL::ArrayList<CPUState *> candidates;
	{
		Sync::ScopedIrqSpinLock guard { m_global_lock };
		for (usize i {}; i < m_cpus.size(); ++i) {
			auto *victim { m_cpus[i] };
			if (!victim || victim == &cpu || !victim->online)
				continue;
			candidates.push(victim);
		}
	}

	for (usize i {}; i < candidates.size(); ++i) {
		auto *victim { candidates[i] };
		if (!victim)
			continue;

		Thread *stolen {};
		{
			Sync::ScopedIrqSpinLock victim_guard { victim->queue_lock };
			for (usize idx { victim->ready_threads.size() };
			    idx > victim->ready_head; --idx) {
				auto slot { idx - 1 };
				auto *candidate { victim->ready_threads[slot] };
				if (!candidate)
					continue;
				if (candidate == victim->idle_thread)
					continue;
				if (candidate->tstate != ThreadState::Ready)
					continue;
				victim->ready_threads[slot] = nullptr;
				stolen = candidate;
				break;
			}
			compact_ready_queue_if_needed(*victim);
		}

		if (stolen) {
			stolen->home_lapic_id = cpu.lapic_id;
			stolen->last_lapic_id = cpu.lapic_id;
			Debug::print_formatted(
			    "[Scheduler] CPU %d stole tid=%d from CPU %d (retargeted).\n",
			    static_cast<int>(cpu.lapic_id),
			    static_cast<int>(stolen->tid.id),
			    static_cast<int>(victim->lapic_id));
			Debug::drain_logs();
			return stolen;
		}
	}

	return nullptr;
}

void Scheduler::reap_zombies()
{
	Sync::ScopedIrqSpinLock guard { m_global_lock };
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
