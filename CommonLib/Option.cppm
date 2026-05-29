export module CommonLib:Option;

import :Platform;
import :StringView;
import :TypeTraits;

export {
	namespace CL {

	template<typename T> struct Option;

	namespace detail {

	template<typename T> struct OptionConstRef {
		using Type = AddConstT<RemoveReferenceT<T>> &;
	};

	template<typename T> struct OptionConstRef<T &> {
		using Type = AddConstT<RemoveReferenceT<T>> &;
	};

	template<typename T>
	using OptionConstRefT = typename OptionConstRef<T>::Type;

	template<typename Derived, typename T> struct OptionAccessBase {
		auto unwrap() -> T &
		{
			if (static_cast<Derived &>(*this).is_none())
				panic("Attempt to get value of empty Option");

			return static_cast<Derived &>(*this).get_unsafe();
		}

		auto expect(StringView const message) -> T &
		{
			if (static_cast<Derived &>(*this).is_none())
				panic(message);

			return static_cast<Derived &>(*this).get_unsafe();
		}

		auto operator*() -> T & { return unwrap(); }

		constexpr auto operator*() const -> OptionConstRefT<T>
		{
			return static_cast<Derived const &>(*this).get_unsafe();
		}

		auto operator->() -> RemoveReferenceT<T> * { return &unwrap(); }

		constexpr auto operator->() const
		{
			return &static_cast<Derived const &>(*this).get_unsafe();
		}
	};

	template<typename Derived, typename T> struct OptionBase {
		constexpr Derived &self() { return static_cast<Derived &>(*this); }
		constexpr Derived const &self() const
		{
			return static_cast<Derived const &>(*this);
		}

		constexpr auto is_none() -> bool { return !self().is_some(); }
		constexpr auto is_none() const -> bool { return !self().is_some(); }

		template<typename Fn> constexpr auto is_some_and(Fn &&fn) -> bool
		{
			return self().is_some() && fn(self().get_unsafe());
		}

		template<typename Fn> constexpr auto is_some_and(Fn &&fn) const -> bool
		{
			return self().is_some() && fn(self().get_unsafe());
		}

		template<typename Fn> constexpr auto is_none_or(Fn &&fn) -> bool
		{
			return is_none() || fn(self().get_unsafe());
		}

		template<typename Fn> constexpr auto is_none_or(Fn &&fn) const -> bool
		{
			return is_none() || fn(self().get_unsafe());
		}

		template<typename Fn> constexpr auto map(Fn &&fn)
		{
			using U = decltype(fn(self().get_unsafe()));

			if (is_none())
				return Option<U>();

			return Option<U>(fn(self().get_unsafe()));
		}

		template<typename Fn> constexpr auto and_then(Fn &&fn)
		{
			using Ret = decltype(fn(self().get_unsafe()));

			if (is_none())
				return Ret();

			return fn(self().get_unsafe());
		}

		template<typename Fn> constexpr Derived &inspect(Fn &&fn)
		{
			if (self().is_some())
				fn(self().get_unsafe());

			return self();
		}

		template<typename Fn> constexpr auto unwrap_or_else(Fn &&fn) -> T
		{
			if (self().is_some())
				return self().get_unsafe();

			return fn();
		}

		constexpr auto unwrap_or(T fallback) -> T
		{
			if (self().is_some())
				return self().get_unsafe();

			return fallback;
		}

		template<typename Fn> constexpr auto or_else(Fn &&fn) -> Derived
		{
			if (self().is_some()) {
				if constexpr (IsReferenceV<T>)
					return self();

				return Derived(self().get_unsafe());
			}

			return fn();
		}
	};

	}

	/// @brief A type that represents an optional value, which may be present or
	/// absent.
	/// @tparam T The type of the value that may be present in the Option.
	/// @code
	/// Option<int> a = 5; // a is an Option that contains the value 5
	/// Option<int> b; // b is an empty Option
	/// if (a) {
	///     // a contains a value, so this block will be executed
	/// }
	/// if (!b) {
	///     // b does not contain a value, so this block will be executed
	/// }
	/// @endcode
	template<typename T>
	struct Option
	    : detail::OptionBase<Option<T>, T>
	    , detail::OptionAccessBase<Option<T>, T> {
		constexpr Option() = default;

		constexpr Option(T value)
		    : m_has_value { true }
		{
			new (&m_union.value) T(static_cast<T &&>(value));
		}

		constexpr Option(Option const &other)
		{
			if (other.m_has_value)
				emplace(other.m_union.value);
		}

		constexpr Option(Option &&other)
		{
			if (other.m_has_value)
				emplace(static_cast<T &&>(other.m_union.value));
		}

		constexpr ~Option() { reset(); }

		constexpr auto operator=(Option const &other) -> Option &
		{
			if (this == &other)
				return *this;

			reset();

			if (other.m_has_value)
				emplace(other.m_union.value);

			return *this;
		}

		constexpr auto operator=(Option &&other) -> Option &
		{
			if (this == &other)
				return *this;

			reset();

			if (other.m_has_value)
				emplace(static_cast<T &&>(other.m_union.value));

			return *this;
		}

		/// @brief Construct a value in place in the Option.
		/// @tparam Args The types of the arguments to construct the value.
		/// @param args The arguments to construct the value.
		/// @return A reference to the constructed value.
		template<typename... Args> constexpr auto emplace(Args &&...args) -> T &
		{
			reset();

			new (&m_union.value) T(static_cast<Args &&>(args)...);
			m_has_value = true;

			return m_union.value;
		}

		/// @brief Reset the Option to an empty state, destroying the contained
		/// value if it is present.
		constexpr auto reset() -> void
		{
			if (m_has_value) {
				m_union.value.~T();
				m_has_value = false;
			}
		}

		/// @brief Get a reference to the value contained in the Option without
		/// checking if it is present.
		/// @return A reference to the value contained in the Option.
		constexpr auto get_unsafe() -> T & { return m_union.value; }
		/// @brief Get a reference to the value contained in the Option without
		/// checking if it is present.
		/// @return A reference to the value contained in the Option.
		constexpr auto get_unsafe() const -> T const & { return m_union.value; }

		/// @brief Check if the Option contains a value.
		/// @return True if the Option contains a value, false otherwise.
		constexpr auto is_some() -> bool { return m_has_value; }
		/// @brief Check if the Option contains a value.
		/// @return True if the Option contains a value, false otherwise.
		constexpr auto is_some() const -> bool { return m_has_value; }

		constexpr explicit operator bool() const { return m_has_value; }

	private:
		bool m_has_value {};
		union Storage {
			constexpr Storage() { }
			constexpr ~Storage() { }

			T value;
		} m_union;
	};

	template<typename T>
	struct Option<T &>
	    : detail::OptionBase<Option<T &>, T &>
	    , detail::OptionAccessBase<Option<T &>, T &> {
		constexpr Option() = default;

		constexpr Option(T &value)
		    : m_value { &value }
		{
		}

		/// @brief Reset the Option to an empty state, destroying the contained
		/// value if it is present.
		constexpr auto reset() -> void { m_value = nullptr; }

		/// @brief Get a reference to the value contained in the Option without
		/// checking if it is present.
		/// @return A reference to the value contained in the Option.
		constexpr auto get_unsafe() -> T & { return *m_value; }
		/// @brief Get a reference to the value contained in the Option without
		/// checking if it is present.
		/// @return A reference to the value contained in the Option.
		constexpr auto get_unsafe() const -> T const & { return *m_value; }

		/// @brief Check if the Option contains a value.
		/// @return True if the Option contains a value, false otherwise.
		constexpr auto is_some() -> bool { return m_value != nullptr; }
		/// @brief Check if the Option contains a value.
		/// @return True if the Option contains a value, false otherwise.
		constexpr auto is_some() const -> bool { return m_value != nullptr; }

		constexpr explicit operator bool() const { return is_some(); }

	private:
		T *m_value { nullptr };
	};

	}
}
