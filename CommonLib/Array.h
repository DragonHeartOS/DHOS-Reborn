#pragma once

#include <CommonLib/InitializerList.h>
#include <CommonLib/Iterator.h>
#include <CommonLib/Option.h>
#include <CommonLib/Types.h>

namespace CL {

template<typename T, usize N> struct Array {
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

	constexpr Array() = default;

	constexpr Array(T const (&data)[N])
	{
		for (usize i = 0; i < N; i++)
			m_data[i] = data[i];
	}

	constexpr Array(Array const &other)
	{
		for (usize i = 0; i < N; i++)
			m_data[i] = other.m_data[i];
	}

	constexpr Array(Array &&other)
	{
		for (usize i = 0; i < N; i++)
			m_data[i] = static_cast<T &&>(other.m_data[i]);
	}

	constexpr Array(InitializerList<T> init)
	{
		usize i = 0;
		for (auto const &value : init) {
			if (i >= N)
				break;
			m_data[i++] = value;
		}
	}

	constexpr auto operator=(Array const &other) -> Array &
	{
		if (this == &other)
			return *this;
		for (usize i = 0; i < N; i++)
			m_data[i] = other.m_data[i];
		return *this;
	}

	constexpr auto operator=(Array &&other) -> Array &
	{
		if (this == &other)
			return *this;
		for (usize i = 0; i < N; i++)
			m_data[i] = static_cast<T &&>(other.m_data[i]);
		return *this;
	}

	auto iter() -> Iter
	{
		return Iter {
			.current = m_data,
			.end = m_data + N,
		};
	}
	auto iter() const -> ConstIter
	{
		return ConstIter {
			.current = m_data,
			.end = m_data + N,
		};
	}

	constexpr auto get(usize index) -> Option<T &>
	{
		if (index >= N)
			return {};
		return m_data[index];
	}

	constexpr auto get(usize index) const -> Option<T const &>
	{
		if (index >= N)
			return {};
		return m_data[index];
	}

	constexpr auto last() -> T & { return m_data[N - 1]; }
	constexpr auto last() const -> T const & { return m_data[N - 1]; }

	constexpr auto size() const -> usize { return N; }

	constexpr auto operator[](usize index) -> T & { return m_data[index]; }
	constexpr auto operator[](usize index) const -> T const &
	{
		return m_data[index];
	}

private:
	T m_data[N];
};

}
