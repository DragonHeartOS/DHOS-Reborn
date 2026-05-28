#pragma once

#include <CommonLib/Option.h>
#include <CommonLib/TypeTraits.h>
#include <CommonLib/Utility.h>

namespace CL {

template<class T>
concept DoubleEndedIterator = requires(T t) {
	t.next();
	t.next_back();
};

template<class Iter, class F> struct MapIter;
template<class Iter, class P> struct FilterIter;
template<DoubleEndedIterator Iter> struct ReverseIter;

template<class Self> struct Iterator {
	Self &self() { return static_cast<Self &>(*this); }

	template<class F> auto map(F f) & = delete ("Iterator must be an rvalue");
	template<class F> auto map(F f) &&
	{
		return MapIter<Self, F>(move(self()), move(f));
	}
	template<class P>
	auto filter(P pred) & = delete ("Iterator must be an rvalue");
	template<class P> auto filter(P pred) &&
	{
		return FilterIter<Self, P>(move(self()), move(pred));
	}
	template<class F>
	void for_each(F f) & = delete ("Iterator must be an rvalue");
	template<class F> void for_each(F f) &&
	{
		while (auto x { self().next() })
			f(*x);
	}

	template<class Container>
	auto collect() & -> Container = delete ("Iterator must be an rvalue");
	template<class Container> auto collect() && -> Container
	{
		Container result;
		while (auto x { self().next() })
			result.push(*x);
		return result;
	}

	auto rev() & = delete ("Iterator must be an rvalue");
	auto rev() &&
	requires DoubleEndedIterator<Self>
	{
		return ReverseIter<Self>(move(self()));
	}

	template<class Other>
	auto eq(Other other) & -> bool = delete ("Iterator must be an rvalue");
	template<class Other> auto eq(Other other) && -> bool
	{
		while (true) {
			auto a { self().next() };
			auto b { other.next() };

			if (!a && !b)
				return true;

			if (!a || !b)
				return false;

			if (*a != *b)
				return false;
		}
	}

	template<class P>
	auto any(P pred) & -> bool = delete ("Iterator must be an rvalue");
	template<class P> auto any(P pred) && -> bool
	{
		return move(self()).find_if(move(pred)).has_value();
	}

	template<class P>
	auto every(P pred) & -> bool = delete ("Iterator must be an rvalue");
	template<class P> auto every(P pred) && -> bool
	{
		while (auto x { self().next() }) {
			if (!pred(*x))
				return false;
		}
		return true;
	}

	template<class P>
	auto find_if(P pred) & = delete ("Iterator must be an rvalue");
	template<class P> auto find_if(P pred) &&
	{
		while (auto x { self().next() }) {
			if (pred(*x))
				return x;
		}
		return decltype(self().next()) {};
	}

	template<class Value>
	auto find(Value const &value) & = delete ("Iterator must be an rvalue");
	template<class Value> auto find(Value const &value) &&
	{
		return move(self()).find_if([&](auto const &x) { return x == value; });
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
		return next_impl([](Iter &iter) { return iter.next(); });
	}

	auto next_back()
	requires DoubleEndedIterator<Iter>
	{
		return next_impl([](Iter &iter) { return iter.next_back(); });
	}

private:
	template<class Next> auto next_impl(Next next)
	{
		auto x { next(iter) };

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
		return next_impl([](Iter &iter) { return iter.next(); });
	}

	auto next_back()
	requires DoubleEndedIterator<Iter>
	{
		return next_impl([](Iter &iter) { return iter.next_back(); });
	}

private:
	template<class Next> auto next_impl(Next next)
	{
		while (auto x { next(iter) }) {
			if (pred(*x))
				return x;
		}

		return decltype(next(iter)) {};
	}
};

template<DoubleEndedIterator Iter>
struct ReverseIter : Iterator<ReverseIter<Iter>> {
	Iter iter;

	ReverseIter(Iter iter)
	    : iter(move(iter))
	{
	}

	auto next() { return iter.next_back(); }

	auto next_back() { return iter.next(); }
};

template<class T>
concept Iterable = requires(T t) { t.iter().next(); };

}
