#pragma once

namespace CL {

template<typename T> struct Box {
	template<typename... Args>
	explicit Box(Args &&...args)
	    : m_data(new T(forward<Args>(args)...))
	{
	}

	~Box() { delete m_data; }

	Box(Box const &) = delete;
	Box &operator=(Box const &) = delete;

	Box(Box &&other) noexcept
	    : m_data(other.m_data)
	{
		other.m_data = nullptr;
	}

	T *leak()
	{
		T *ptr = m_data;
		m_data = nullptr;
		return ptr;
	}

	Box &operator=(Box &&other) noexcept
	{
		if (this != &other) {
			delete m_data;
			m_data = other.m_data;
			other.m_data = nullptr;
		}
		return *this;
	}

	T &operator*() { return *m_data; }
	T const &operator*() const { return *m_data; }

	T *operator->() { return m_data; }
	T const *operator->() const { return m_data; }

	T *get() { return m_data; }
	T const *get() const { return m_data; }

private:
	T *m_data = nullptr;
};

}
