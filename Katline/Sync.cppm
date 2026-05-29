export module Katline:Sync;

import CommonLib;

export {
	namespace Katline::Sync {

	using CL::SpinLock;

	auto irq_save_disable() -> u64;
	auto irq_restore(u64 flags) -> void;

	class ScopedIrqSpinLock {
	public:
		explicit ScopedIrqSpinLock(SpinLock &lock);
		ScopedIrqSpinLock(ScopedIrqSpinLock const &) = delete;
		auto operator=(ScopedIrqSpinLock const &)
		    -> ScopedIrqSpinLock & = delete;
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

ScopedIrqSpinLock::ScopedIrqSpinLock(SpinLock &lock)
    : m_lock { lock }
    , m_flags { irq_save_disable() }
{
	m_lock.lock();
}

ScopedIrqSpinLock::~ScopedIrqSpinLock()
{
	m_lock.unlock();
	irq_restore(m_flags);
}

}
