#pragma once

#include <CommonLib/InitializerList.h>
#include <CommonLib/Iterator.h>
#include <CommonLib/Option.h>
#include <CommonLib/Platform.h>
#include <CommonLib/Types.h>

namespace CL {

template<typename T> struct ArrayList {
	struct Iter : Iterator<Iter> {
		T *current;
		T *end;

		auto next() -> Option<T &>
		{
			if (current == end)
				return {};

			return *current++;
		}

		auto next_back() -> Option<T &>
		{
			if (current == end)
				return {};

			end--;
			return *end;
		}
	};

	struct ConstIter : Iterator<ConstIter> {
		T const *current;
		T const *end;

		auto next() -> Option<T const &>
		{
			if (current == end)
				return {};

			return *current++;
		}

		auto next_back() -> Option<T const &>
		{
			if (current == end)
				return {};

			end--;
			return *end;
		}
	};

	constexpr ArrayList() = default;

	ArrayList(ArrayList const &other)
	{
		reserve(other.m_size);
		for (usize i = 0; i < other.m_size; ++i)
			emplace(other.m_data[i]);
	}

	ArrayList(ArrayList &&other)
	    : m_data(other.m_data)
	    , m_size(other.m_size)
	    , m_capacity(other.m_capacity)
	{
		other.m_data = nullptr;
		other.m_size = 0;
		other.m_capacity = 0;
	}

	ArrayList(std::initializer_list<T> init)
	{
		reserve(init.size());
		for (usize i = 0; i < init.size(); ++i)
			emplace(init[i]);
	}

	~ArrayList()
	{
		clear();
		deallocate(m_data);
	}

	auto iter() -> Iter
	{
		return Iter {
			.current = m_data,
			.end = m_data + m_size,
		};
	}

	auto iter() const -> ConstIter
	{
		return ConstIter {
			.current = m_data,
			.end = m_data + m_size,
		};
	}

	auto operator=(ArrayList const &other) -> ArrayList &
	{
		if (this == &other)
			return *this;

		clear();
		reserve(other.m_size);

		for (usize i = 0; i < other.m_size; ++i)
			emplace(other.m_data[i]);

		return *this;
	}

	auto operator=(ArrayList &&other) -> ArrayList &
	{
		if (this == &other)
			return *this;

		clear();
		deallocate(m_data);

		m_data = other.m_data;
		m_size = other.m_size;
		m_capacity = other.m_capacity;

		other.m_data = nullptr;
		other.m_size = 0;
		other.m_capacity = 0;

		return *this;
	}

	auto push(T const &value) -> void { emplace(value); }
	auto push(T &&value) -> void { emplace(static_cast<T &&>(value)); }

	template<typename... Args> auto emplace(Args &&...args) -> T &
	{
		ensure_capacity(m_size + 1);

		new (&m_data[m_size]) T(static_cast<Args &&>(args)...);
		return m_data[m_size++];
	}

	auto pop() -> void
	{
		if (m_size == 0)
			return;

		--m_size;
		m_data[m_size].~T();
	}

	auto clear() -> void
	{
		for (usize i = 0; i < m_size; ++i)
			m_data[i].~T();

		m_size = 0;
	}

	auto reserve(usize capacity) -> void
	{
		if (capacity <= m_capacity)
			return;

		T *new_data = allocate(capacity);

		for (usize i = 0; i < m_size; ++i)
			new (&new_data[i]) T(static_cast<T &&>(m_data[i]));

		for (usize i = 0; i < m_size; ++i)
			m_data[i].~T();

		deallocate(m_data);

		m_data = new_data;
		m_capacity = capacity;
	}

	auto remove_at(usize index) -> void
	{
		if (index >= m_size)
			return;

		m_data[index].~T();

		for (usize i = index; i + 1 < m_size; ++i) {
			new (&m_data[i]) T(static_cast<T &&>(m_data[i + 1]));
			m_data[i + 1].~T();
		}

		--m_size;
	}

	constexpr auto get(usize index) -> Option<T &>
	{
		if (index >= m_size)
			return {};

		return m_data[index];
	}

	constexpr auto get(usize index) const -> Option<T const &>
	{
		if (index >= m_size)
			return {};

		return m_data[index];
	}

	constexpr auto last() -> T & { return m_data[m_size - 1]; }
	constexpr auto last() const -> T const & { return m_data[m_size - 1]; }

	constexpr auto size() const -> usize { return m_size; }
	constexpr auto capacity() const -> usize { return m_capacity; }
	constexpr auto is_empty() const -> bool { return m_size == 0; }

	constexpr auto data() -> T * { return m_data; }
	constexpr auto data() const -> T const * { return m_data; }

	constexpr auto operator[](usize index) -> T & { return m_data[index]; }
	constexpr auto operator[](usize index) const -> T const &
	{
		return m_data[index];
	}

protected:
	static auto allocate(usize capacity) -> T *
	{
		return static_cast<T *>(::operator new(sizeof(T) * capacity));
	}

	static auto deallocate(T *data) -> void { ::operator delete(data); }

	auto ensure_capacity(usize wanted) -> void
	{
		if (wanted <= m_capacity)
			return;

		usize new_capacity = m_capacity ? m_capacity * 2 : 8;

		if (new_capacity < wanted)
			new_capacity = wanted;

		reserve(new_capacity);
	}

	T *m_data { nullptr };
	usize m_size { 0 };
	usize m_capacity { 0 };
};

}
