export module Katline:Sync;

import CommonLib;

export {
	namespace Katline::Sync {

	auto irq_save_disable() -> u64;
	auto irq_restore(u64 flags) -> void;

	class SpinLock {
	public:
		auto lock() -> void;
		auto unlock() -> void;

	private:
		CL::Atomic<bool> m_locked { false };
	};

	class ScopedSpinLock {
	public:
		explicit ScopedSpinLock(SpinLock &lock);
		ScopedSpinLock(ScopedSpinLock const &) = delete;
		auto operator=(ScopedSpinLock const &) -> ScopedSpinLock & = delete;
		~ScopedSpinLock();

	private:
		SpinLock &m_lock;
	};

	class ScopedIrqSpinLock {
	public:
		explicit ScopedIrqSpinLock(SpinLock &lock);
		ScopedIrqSpinLock(ScopedIrqSpinLock const &) = delete;
		auto operator=(ScopedIrqSpinLock const &) -> ScopedIrqSpinLock & = delete;
		~ScopedIrqSpinLock();

	private:
		SpinLock &m_lock;
		u64 m_flags {};
	};

	}
}

namespace Katline::Sync {

auto irq_save_disable() -> u64
{
	u64 flags {};
	asm volatile("pushfq\n\tpopq %0\n\tcli" : "=r"(flags) : : "memory");
	return flags;
}

auto irq_restore(u64 flags) -> void
{
	asm volatile("pushq %0\n\tpopfq" : : "r"(flags) : "memory", "cc");
}

auto SpinLock::lock() -> void
{
	while (m_locked.exchange(true, CL::MemoryOrder::Acquire))
		asm volatile("pause");
}

auto SpinLock::unlock() -> void
{
	m_locked.store(false, CL::MemoryOrder::Release);
}

ScopedSpinLock::ScopedSpinLock(SpinLock &lock)
    : m_lock { lock }
{
	m_lock.lock();
}

ScopedSpinLock::~ScopedSpinLock()
{
	m_lock.unlock();
}

ScopedIrqSpinLock::ScopedIrqSpinLock(SpinLock &lock)
    : m_lock { lock }, m_flags { irq_save_disable() }
{
	m_lock.lock();
}

ScopedIrqSpinLock::~ScopedIrqSpinLock()
{
	m_lock.unlock();
	irq_restore(m_flags);
}

}
