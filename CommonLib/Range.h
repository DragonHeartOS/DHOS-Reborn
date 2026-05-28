#pragma once

#include <CommonLib/Iterator.h>

namespace CL {

/// @brief An iterator that produces a sequence of values from a starting point
/// to an endpoint.
/// @tparam T The type of the values produced by the iterator.
///
/// @code
/// for (auto i : range(0, 10)) {
///     // i will take on the values 0, 1, 2, ..., 9
/// }
/// @endcode
template<class T> struct Range : Iterator<Range<T>> {
	T current;
	T end_;

	Range(T start, T end)
	    : current(start)
	    , end_(end)
	{
	}

	auto iter() { return move(*this); }

	auto next() -> Option<T>
	{
		if (current >= end_)
			return {};

		return current++;
	}

	auto next_back() -> Option<T>
	{
		if (current >= end_)
			return {};

		return --end_;
	}
};

/// @brief Create a new Range iterator.
/// @tparam T The type of the values produced by the iterator.
/// @param start The starting value of the range.
/// @param end The ending value of the range.
/// @return A new Range iterator.
///
/// @code
/// for (auto i : range(4, 10)) {
///     // i will take on the values 4, 5, 6, 7, 8, 9
/// }
/// @endcode
template<class T> auto range(T start, T end) { return Range<T> { start, end }; }

/// @brief Create a new Range iterator from 0 to the specified endpoint.
/// @tparam T The type of the values produced by the iterator.
/// @param end The ending value of the range.
/// @return A new Range iterator.
/// @code
/// for (auto i : range(10)) {
///     // i will take on the values 0, 1, 2, ..., 9
/// }
/// @endcode
template<class T> auto range(T end) { return Range<T> { T {}, end }; }

}
