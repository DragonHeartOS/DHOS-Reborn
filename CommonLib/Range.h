#pragma once

#include <CommonLib/Iterator.h>

namespace CL {

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

template<class T> auto range(T start, T end) { return Range<T> { start, end }; }

template<class T> auto range(T end) { return Range<T> { T {}, end }; }

}
