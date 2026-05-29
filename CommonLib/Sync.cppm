export module CommonLib:Sync;

import :Atomic;

export {
	namespace CL {

	class SpinLock {
	public:
		auto lock() -> void;
		auto unlock() -> void;

	private:
		Atomic<bool> m_locked { false };
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

	}
}

namespace CL {

auto SpinLock::lock() -> void
{
	while (m_locked.exchange(true, MemoryOrder::Acquire)) { }
}

auto SpinLock::unlock() -> void { m_locked.store(false, MemoryOrder::Release); }

ScopedSpinLock::ScopedSpinLock(SpinLock &lock)
    : m_lock { lock }
{
	m_lock.lock();
}

ScopedSpinLock::~ScopedSpinLock() { m_lock.unlock(); }

}
