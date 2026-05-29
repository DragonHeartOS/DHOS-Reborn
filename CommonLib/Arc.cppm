export module CommonLib:Arc;

import :Atomic;
import :Types;
import :Utility;

export {
	namespace CL {

	/// @brief An atomically reference-counted smart pointer that owns a
	/// heap-allocated object of type T.
	/// @tparam T The type of the object owned by the Arc.
	template<typename T> struct Arc {
		template<typename... Args>
		explicit Arc(Args &&...args)
		    : m_ptr { new T(forward<Args>(args)...) }
		    , m_refs { new Atomic<usize>(1) }
		{
		}

		~Arc() { release(); }

		Arc(Arc const &other)
		    : m_ptr { other.m_ptr }
		    , m_refs { other.m_refs }
		{
			if (m_refs)
				m_refs->fetch_add(1, MemoryOrder::Relaxed);
		}

		Arc &operator=(Arc const &other)
		{
			if (this != &other) {
				release();

				m_ptr = other.m_ptr;
				m_refs = other.m_refs;

				if (m_refs)
					m_refs->fetch_add(1, MemoryOrder::Relaxed);
			}

			return *this;
		}

		Arc(Arc &&other) noexcept
		    : m_ptr { other.m_ptr }
		    , m_refs { other.m_refs }
		{
			other.m_ptr = nullptr;
			other.m_refs = nullptr;
		}

		Arc &operator=(Arc &&other) noexcept
		{
			if (this != &other) {
				release();

				m_ptr = other.m_ptr;
				m_refs = other.m_refs;

				other.m_ptr = nullptr;
				other.m_refs = nullptr;
			}

			return *this;
		}

		T &operator*() { return *m_ptr; }
		T const &operator*() const { return *m_ptr; }

		T *operator->() { return m_ptr; }
		T const *operator->() const { return m_ptr; }

		/// @brief Get a pointer to the owned object.
		/// @return A pointer to the owned object.
		T *get() { return m_ptr; }
		/// @brief Get a pointer to the owned object.
		/// @return A pointer to the owned object.
		T const *get() const { return m_ptr; }

		auto ref_count() const -> usize
		{
			return m_refs ? m_refs->load(MemoryOrder::Relaxed) : 0;
		}

	private:
		auto release() -> void
		{
			if (!m_refs)
				return;

			if (m_refs->fetch_sub(1, MemoryOrder::AcquireRelease) == 1) {
				delete m_ptr;
				delete m_refs;
			}

			m_ptr = nullptr;
			m_refs = nullptr;
		}

	private:
		T *m_ptr { nullptr };
		Atomic<usize> *m_refs { nullptr };
	};

	}
}
