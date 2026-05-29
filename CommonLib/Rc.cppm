export module CommonLib:Rc;

import :Types;
import :Utility;

export {
	namespace CL {

	/// @brief A reference-counted smart pointer for managing shared ownership
	/// of a value of type T.
	/// @tparam T The type of the value being managed by the Rc.
	template<typename T> struct Rc {
		template<typename... Args>
		explicit Rc(Args &&...args)
		    : m_ptr { new T(forward<Args>(args)...) }
		    , m_refs { new usize(1) }
		{
		}

		~Rc() { release(); }

		Rc(Rc const &other)
		    : m_ptr { other.m_ptr }
		    , m_refs { other.m_refs }
		{
			if (m_refs)
				++(*m_refs);
		}

		Rc &operator=(Rc const &other)
		{
			if (this != &other) {
				release();

				m_ptr = other.m_ptr;
				m_refs = other.m_refs;

				if (m_refs)
					++(*m_refs);
			}

			return *this;
		}

		Rc(Rc &&other) noexcept
		    : m_ptr { other.m_ptr }
		    , m_refs { other.m_refs }
		{
			other.m_ptr = nullptr;
			other.m_refs = nullptr;
		}

		Rc &operator=(Rc &&other) noexcept
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

		// @brief Get a pointer to the managed value.
		// @return A pointer to the managed value.
		T *get() { return m_ptr; }
		T const *get() const { return m_ptr; }

		/// @brief Get the number of references to the managed value.
		/// @return The number of references to the managed value.
		auto ref_count() const -> usize { return m_refs ? *m_refs : 0; }

	private:
		auto release() -> void
		{
			if (!m_refs)
				return;

			--(*m_refs);

			if (*m_refs == 0) {
				delete m_ptr;
				delete m_refs;
			}

			m_ptr = nullptr;
			m_refs = nullptr;
		}

	private:
		T *m_ptr { nullptr };
		usize *m_refs { nullptr };
	};

	}
}
