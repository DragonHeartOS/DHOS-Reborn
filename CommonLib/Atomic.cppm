export module CommonLib:Atomic;
export {
	namespace CL {

	/// @brief Memory order for atomic operations.
	enum class MemoryOrder {
		/// @brief No ordering constraints.
		Relaxed = __ATOMIC_RELAXED,
		/// @brief Acquire ordering constraints.
		Acquire = __ATOMIC_ACQUIRE,
		/// @brief Release ordering constraints.
		Release = __ATOMIC_RELEASE,
		/// @brief Acquire-release ordering constraints.
		AcquireRelease = __ATOMIC_ACQ_REL,
		/// @brief Sequentially consistent ordering constraints.
		SequentiallyConsistant = __ATOMIC_SEQ_CST,
	};

	/// @brief An atomic variable of type T.
	/// @tparam T The type of the atomic variable.
	template<typename T> struct Atomic {
		Atomic() = default;

		constexpr Atomic(T value)
		    : m_value(value)
		{
		}

		Atomic(Atomic const &) = delete;
		Atomic &operator=(Atomic const &) = delete;

		/// @brief Load the value of the atomic variable.
		/// @param order The memory order for the load operation.
		/// @return The value of the atomic variable.
		auto load(MemoryOrder order = MemoryOrder::SequentiallyConsistant) const
		    -> T
		{
			return __atomic_load_n(&m_value, (int)order);
		}

		/// @brief Store a value to the atomic variable.
		/// @param value The value to store.
		/// @param order The memory order for the store operation.
		auto store(T value,
		    MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> void
		{
			__atomic_store_n(&m_value, value, (int)order);
		}

		auto exchange(T value,
		    MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
		{
			return __atomic_exchange_n(&m_value, value, (int)order);
		}

		/// @brief Compare the value of the atomic variable with an expected
		/// value and exchange it if they match.
		/// @param expected A reference to the expected value.
		/// @param desired The value to exchange if the comparison succeeds.
		/// @param success The memory order for the operation if the comparison
		/// succeeds.
		/// @param failure The memory order for the operation if the comparison
		/// fails.
		/// @return True if the comparison succeeded and the exchange was
		/// performed, false otherwise.
		auto compare_exchange(T &expected, T desired,
		    MemoryOrder success = MemoryOrder::SequentiallyConsistant,
		    MemoryOrder failure = MemoryOrder::SequentiallyConsistant) -> bool
		{
			return __atomic_compare_exchange_n(&m_value, &expected, desired,
			    false, (int)success, (int)failure);
		}

		/// @brief Atomically add a value to the atomic variable and return the
		/// previous value.
		/// @param value The value to add.
		/// @param order The memory order for the operation.
		/// @return The previous value of the atomic variable.
		auto fetch_add(T value,
		    MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
		{
			return __atomic_fetch_add(&m_value, value, (int)order);
		}

		/// @brief Atomically subtract a value from the atomic variable and
		/// return the previous value.
		/// @param value The value to subtract.
		/// @param order The memory order for the operation.
		/// @return The previous value of the atomic variable.
		auto fetch_sub(T value,
		    MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
		{
			return __atomic_fetch_sub(&m_value, value, (int)order);
		}

		/// @brief Atomically perform a bitwise OR operation on the atomic
		/// variable and return the previous value.
		/// @param value The value to OR with.
		/// @param order The memory order for the operation.
		/// @return The previous value of the atomic variable.
		auto fetch_or(T value,
		    MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
		{
			return __atomic_fetch_or(&m_value, value, (int)order);
		}

		/// @brief Atomically perform a bitwise AND operation on the atomic
		/// variable and return the previous value.
		/// @param value The value to AND with.
		/// @param order The memory order for the operation.
		/// @return The previous value of the atomic variable.
		auto fetch_and(T value,
		    MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
		{
			return __atomic_fetch_and(&m_value, value, (int)order);
		}

		/// @brief Atomically perform a bitwise XOR operation on the atomic
		/// variable and return the previous value.
		/// @param value The value to XOR with.
		/// @param order The memory order for the operation.
		/// @return The previous value of the atomic variable.
		auto fetch_xor(T value,
		    MemoryOrder order = MemoryOrder::SequentiallyConsistant) -> T
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
}
