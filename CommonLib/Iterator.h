#pragma once

#include <CommonLib/Option.h>
#include <CommonLib/TypeTraits.h>
#include <CommonLib/Utility.h>

namespace CL {

template<class Iter, class F> struct MapIter;

template<class Iter, class P> struct FilterIter;

template<class Self> struct Iterator {
	Self &self() { return static_cast<Self &>(*this); }

	template<class F> auto map(F f) &
	{
		return MapIter<Self, F>(move(self()), move(f));
	}
	template<class F> auto map(F f) &&
	{
		return MapIter<Self, F>(move(self()), move(f));
	}

	template<class P> auto filter(P pred) &
	{
		return FilterIter<Self, P>(move(self()), move(pred));
	}
	template<class P> auto filter(P pred) &&
	{
		return FilterIter<Self, P>(move(self()), move(pred));
	}

	template<class F> void for_each(F f) &
	{
		while (auto x { self().next() }) {
			f(*x);
		}
	}
	template<class F> void for_each(F f) &&
	{
		while (auto x { self().next() }) {
			f(*x);
		}
	}

	template<class Container> auto collect() -> Container
	{
		Container result;

		while (auto x { self().next() }) {
			result.push(*x);
		}

		return result;
	}
};

template<class Iter, class F> struct MapIter : Iterator<MapIter<Iter, F>> {
	Iter iter;
	F f;

	MapIter(Iter iter, F f)
	    : iter(move(iter))
	    , f(move(f))
	{
	}

	auto next()
	{
		auto x { iter.next() };

		using Out = InvokeResultT<F, decltype(*x)>;

		if (!x)
			return Option<Out> {};

		return Option<Out> { f(*x) };
	}
};

template<class Iter, class P>
struct FilterIter : Iterator<FilterIter<Iter, P>> {
	Iter iter;
	P pred;

	FilterIter(Iter iter, P pred)
	    : iter(move(iter))
	    , pred(move(pred))
	{
	}

	auto next()
	{
		while (auto x = iter.next()) {
			if (pred(*x))
				return x;
		}

		return decltype(iter.next()) {};
	}
};

template<class T>
concept Iterable = requires(T t) { t.iter(); };

}
