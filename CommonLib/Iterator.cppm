export module CommonLib:Iterator;

import :Option;
import :Pair;
import :TypeTraits;
import :Utility;

export {
	namespace CL {

	template<class T>
	concept DoubleEndedIterator = requires(T t) {
		t.next();
		t.next_back();
	};

	template<class Iter, class F> struct MapIter;
	template<class Iter, class P> struct FilterIter;
	template<class Iter> struct EnumerateIter;
	template<DoubleEndedIterator Iter> struct ReverseIter;

	/// @brief A base class for iterators that provides common iterator methods
	/// such as map, filter, collect, etc.
	/// @tparam Self The type of the derived iterator class.
	template<class Self> struct Iterator {
		Self &self() { return static_cast<Self &>(*this); }

		template<class F>
		auto map(F f) & = delete ("Iterator must be an rvalue");
		/// @brief Create a new iterator that applies a function to each element
		/// of the original iterator.
		/// @tparam F The type of the function to apply to each element of the
		/// original iterator.
		/// @param f The function to apply to each element of the original
		/// iterator.
		/// @return A new iterator that applies the function to each element of
		/// the original iterator.
		///
		/// @code
		/// auto doubled { range(3).map([](auto x) { return x * 2; }) };
		/// @endcode
		template<class F> auto map(F f) &&
		{
			return MapIter<Self, F>(move(self()), move(f));
		}
		template<class P>
		auto filter(P pred) & = delete ("Iterator must be an rvalue");
		/// @brief Create a new iterator that filters the elements of the
		/// original iterator using a predicate function.
		/// @tparam P The type of the predicate function to use for filtering
		/// the elements of the original iterator.
		/// @param pred The predicate function to use for filtering the elements
		/// of the original iterator.
		/// @return A new iterator that filters the elements of the original
		/// iterator using the specified predicate function.
		///
		/// @code
		/// auto evens { range(6).filter([](auto x) { return x % 2 == 0; }) };
		/// @endcode
		template<class P> auto filter(P pred) &&
		{
			return FilterIter<Self, P>(move(self()), move(pred));
		}
		auto enumerate() & = delete ("Iterator must be an rvalue");
		/// @brief Create a new iterator that yields `(index, value)` pairs.
		/// @return A new iterator that yields the zero-based index with each
		/// element from the original iterator.
		///
		/// @code
		/// auto indexed { range(3).enumerate() };
		/// @endcode
		auto enumerate() && { return EnumerateIter<Self>(move(self())); }
		template<class F>
		void for_each(F f) & = delete ("Iterator must be an rvalue");
		/// @brief Apply a function to each element of the iterator.
		/// @tparam F The type of the function to apply to each element of the
		/// iterator.
		/// @param f The function to apply to each element of the iterator.
		///
		/// @code
		/// range(3).for_each([](auto x) {
		///     // use x
		/// });
		/// @endcode
		template<class F> void for_each(F f) &&
		{
			while (auto x { self().next() })
				f(*x);
		}

		template<class Container>
		auto collect() & -> Container = delete ("Iterator must be an rvalue");
		/// @brief Collect the elements of the iterator into a container.
		/// @tparam Container The type of the container to collect the elements
		/// into. The container must have a push method that can be used to add
		/// elements to the container.
		/// @param container The container to collect the elements into.
		/// @return The container with the collected elements.
		///
		/// @code
		/// auto values { range(3).collect<ArrayList<int>>() };
		/// @endcode
		template<template<class...> class Container> auto collect() &&
		{
			using Item = RemoveConstRef<decltype(*declval<Self>().next())>;
			Container<Item> result;

			while (auto x { self().next() })
				result.push(*x);

			return result;
		}

		auto rev() & = delete ("Iterator must be an rvalue");
		/// @brief Create a new iterator that iterates over the elements of the
		/// original iterator in reverse order.
		/// @return A new iterator that iterates over the elements of the
		/// original iterator in reverse order.
		///
		/// @code
		/// auto reversed { range(3).rev() };
		/// @endcode
		auto rev() &&
		requires DoubleEndedIterator<Self>
		{
			return ReverseIter<Self>(move(self()));
		}

		template<class Other>
		auto eq(Other other) & -> bool = delete ("Iterator must be an rvalue");
		/// @brief Check if the elements of the iterator are equal to the
		/// elements of another iterator.
		/// @tparam Other The type of the other iterator to compare with. The
		/// other iterator must produce elements of the same type as the
		/// original iterator.
		/// @param other The other iterator to compare with.
		/// @return True if the elements of the original iterator are equal to
		/// the elements of the other iterator, false otherwise.
		///
		/// @code
		/// bool same { range(3).eq(range(3)) };
		/// @endcode
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
		/// @brief Check if any element of the iterator satisfies a predicate
		/// function.
		/// @tparam P The type of the predicate function to use for checking the
		/// elements of the iterator.
		/// @param pred The predicate function to use for checking the elements
		/// of the iterator.
		/// @return True if any element of the iterator satisfies the predicate
		/// function, false otherwise.
		///
		/// @code
		/// bool has_even { range(3).any([](auto x) { return x % 2 == 0; }) };
		/// @endcode
		template<class P> auto any(P pred) && -> bool
		{
			return move(self()).find_if(move(pred)).is_some();
		}

		template<class P>
		auto every(P pred) & -> bool = delete ("Iterator must be an rvalue");
		/// @brief Check if every element of the iterator satisfies a predicate
		/// function.
		/// @tparam P The type of the predicate function to use for checking the
		/// elements of the iterator.
		/// @param pred The predicate function to use for checking the elements
		/// of the iterator.
		/// @return True if every element of the iterator satisfies the
		/// predicate function, false otherwise.
		///
		/// @code
		/// bool all_even { range(3).every([](auto x) { return x % 2 == 0; }) };
		/// @endcode
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
		/// @brief Find the first element of the iterator that satisfies a
		/// predicate function.
		/// @tparam P The type of the predicate function to use for finding the
		/// element of the iterator.
		/// @param pred The predicate function to use for finding the element of
		/// the iterator.
		/// @return An Option containing the first element of the iterator that
		/// satisfies the predicate function if such an element exists, or an
		/// empty Option if no such element exists.
		///
		/// @code
		/// auto first_even { range(5).find_if([](auto x) { return x % 2 == 0;
		/// }) };
		/// @endcode
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
		/// @brief Find the first element of the iterator that is equal to a
		/// specified value.
		/// @tparam Value The type of the value to find in the iterator. The
		/// value must be comparable with the elements of the iterator using the
		/// equality operator.
		/// @param value The value to find in the iterator.
		/// @return An Option containing the first element of the iterator that
		/// is equal to the specified value if such an element exists, or an
		/// empty Option if no such element exists.
		///
		/// @code
		/// auto found { range(5).find(3) };
		/// @endcode
		template<class Value> auto find(Value const &value) &&
		{
			return move(self()).find_if(
			    [&](auto const &x) { return x == value; });
		}
	};

	template<class Iter, class F> struct MapIter : Iterator<MapIter<Iter, F>> {
		Iter iter;
		F f;

		MapIter(Iter iter, F f)
		    : iter { move(iter) }
		    , f { move(f) }
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
		    : iter { move(iter) }
		    , pred { move(pred) }
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

	template<class Iter> struct EnumerateIter : Iterator<EnumerateIter<Iter>> {
		Iter iter;
		usize index {};

		EnumerateIter(Iter iter)
		    : iter { move(iter) }
		{
		}

		auto next()
		{
			auto x { iter.next() };

			if (!x)
				return Option<Pair<usize, decltype(*x)>> {};

			return Option<Pair<usize, decltype(*x)>> {
				Pair<usize, decltype(*x)> { index++, *x },
			};
		}
	};

	template<DoubleEndedIterator Iter>
	struct ReverseIter : Iterator<ReverseIter<Iter>> {
		Iter iter;

		ReverseIter(Iter iter)
		    : iter { move(iter) }
		{
		}

		auto next() { return iter.next_back(); }

		auto next_back() { return iter.next(); }
	};

	template<class T>
	concept Iterable = requires(T t) { t.iter().next(); };

	}
}
