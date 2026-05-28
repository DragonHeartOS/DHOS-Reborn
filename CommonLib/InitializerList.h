#pragma once

#include <CommonLib/Types.h>

namespace std {

template<typename T> class initializer_list {
public:
	using value_type = T;
	using reference = T const &;
	using const_reference = T const &;
	using size_type = usize;

	using iterator = T const *;
	using const_iterator = T const *;

	constexpr initializer_list() noexcept
	    : m_begin(nullptr)
	    , m_size(0)
	{
	}

	constexpr auto size() const noexcept -> size_type { return m_size; }
	constexpr auto begin() const noexcept -> const_iterator { return m_begin; }
	constexpr auto end() const noexcept -> const_iterator
	{
		return m_begin + m_size;
	}
	constexpr auto data() const noexcept -> T const * { return m_begin; }

	constexpr auto operator[](usize index) const -> T const &
	{
		return m_begin[index];
	}

private:
	constexpr initializer_list(T const *begin, size_type size) noexcept
	    : m_begin(begin)
	    , m_size(size)
	{
	}

	T const *m_begin;
	size_type m_size;
};

}

namespace CL {
template<typename T> using InitializerList = std::initializer_list<T>;
}
