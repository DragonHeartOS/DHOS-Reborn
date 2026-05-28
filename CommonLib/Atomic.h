#pragma once

namespace CL {

enum class MemoryOrder {
	Relaxed = __ATOMIC_RELAXED,
	Acquire = __ATOMIC_ACQUIRE,
	Release = __ATOMIC_RELEASE,
	AcquireRelease = __ATOMIC_ACQ_REL,
	SequentiallyConsistant = __ATOMIC_SEQ_CST,
};

template<typename T> struct Atomic {
	Atomic() = default;

	constexpr Atomic(T value)
	    : m_value(value)
	{
	}

	Atomic(Atomic const &) = delete;
	Atomic &operator=(Atomic const &) = delete;

	auto load(MemoryOrder order = MemoryOrder::SequentiallyConsistant) const
	    -> T
	{
		return __atomic_load_n(&m_value, (int)order);
	}

	auto store(T value, MemoryOrder order = MemoryOrder::SequentiallyConsistant)
	    -> void
	{
		__atomic_store_n(&m_value, value, (int)order);
	}

	auto exchange(
	    T value, MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
	{
		return __atomic_exchange_n(&m_value, value, (int)order);
	}

	auto compare_exchange(T &expected, T desired,
	    MemoryOrder success = MemoryOrder::SequentiallyConsistant,
	    MemoryOrder failure = MemoryOrder::SequentiallyConsistant) -> bool
	{
		return __atomic_compare_exchange_n(
		    &m_value, &expected, desired, false, (int)success, (int)failure);
	}

	auto fetch_add(
	    T value, MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
	{
		return __atomic_fetch_add(&m_value, value, (int)order);
	}

	auto fetch_sub(
	    T value, MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
	{
		return __atomic_fetch_sub(&m_value, value, (int)order);
	}

	auto fetch_or(
	    T value, MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
	{
		return __atomic_fetch_or(&m_value, value, (int)order);
	}

	auto fetch_and(
	    T value, MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
	{
		return __atomic_fetch_and(&m_value, value, (int)order);
	}

	auto fetch_xor(
	    T value, MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
	{
		return __atomic_fetch_xor(&m_value, value, (int)order);
	}

	operator T() const { return load(); }

	auto operator=(T value) -> T
	{
		store(value);
		return value;
	}

private:
	mutable T m_value {};
};

}
