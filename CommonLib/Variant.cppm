export module CommonLib:Variant;

import :Option;
import :TypeTraits;
import :Utility;

export {
	namespace CL {

	namespace detail {

	template<typename... Ts> union VariantStorage;

	template<typename T> union VariantStorage<T> {
		VariantStorage() { }
		~VariantStorage() { }

		T value;
	};

	template<typename T, typename... Ts> union VariantStorage<T, Ts...> {
		VariantStorage() { }
		~VariantStorage() { }

		T value;
		VariantStorage<Ts...> next;
	};

	template<int I, typename Storage> struct VariantGet;

	template<typename T, typename... Ts>
	struct VariantGet<0, VariantStorage<T, Ts...>> {
		using Type = T;

		static auto get(VariantStorage<T, Ts...> &s) -> Type &
		{
			return s.value;
		}
		static auto get(VariantStorage<T, Ts...> const &s) -> Type const &
		{
			return s.value;
		}
	};

	template<int I, typename T, typename... Ts>
	struct VariantGet<I, VariantStorage<T, Ts...>> {
		using Type = typename VariantGet<I - 1, VariantStorage<Ts...>>::Type;

		static auto get(VariantStorage<T, Ts...> &s) -> Type &
		{
			return VariantGet<I - 1, VariantStorage<Ts...>>::get(s.next);
		}

		static auto get(VariantStorage<T, Ts...> const &s) -> Type const &
		{
			return VariantGet<I - 1, VariantStorage<Ts...>>::get(s.next);
		}
	};

	template<typename T, typename... Ts> struct VariantIndex;

	template<typename T, typename... Ts> struct VariantIndex<T, T, Ts...> {
		static constexpr int value = 0;
	};

	template<typename T, typename U, typename... Ts>
	struct VariantIndex<T, U, Ts...> {
		static constexpr int value = 1 + VariantIndex<T, Ts...>::value;
	};

	}

	/// @brief A typed union that can hold a value of one of the specified
	/// types.
	/// @tparam Ts The types that the Variant can hold.
	template<typename... Ts> struct Variant {
		template<typename T, typename... Args>
		explicit Variant(InPlace<T>, Args &&...args)
		{
			constexpr int I = detail::VariantIndex<T, Ts...>::value;
			construct<I>(forward<Args>(args)...);
		}

		template<typename T>
		requires OneOf<RemoveConstRef<T>, Ts...> constexpr Variant(T &&value)
		{
			using U = RemoveConstRef<T>;
			constexpr int I = detail::VariantIndex<U, Ts...>::value;
			construct<I>(forward<T>(value));
		}

		/// @brief Create a Variant holding a value of the specified type
		/// constructed with the given arguments.
		/// @tparam I The index of the type to construct in the Variant.
		/// @tparam Args The types of the arguments to construct the value.
		/// @param args The arguments to construct the value.
		template<int I, typename... Args>
		static auto make(Args &&...args) -> Variant
		{
			Variant v;
			v.construct<I>(forward<Args>(args)...);
			return v;
		}

		~Variant() { destroy(); }

		Variant(Variant const &other) { copy_from(other); }

		auto operator=(Variant const &other) -> Variant &
		{
			if (this != &other) {
				destroy();
				copy_from(other);
			}

			return *this;
		}

		Variant(Variant &&other) { move_from(other); }

		auto operator=(Variant &&other) -> Variant &
		{
			if (this != &other) {
				destroy();
				move_from(other);
			}

			return *this;
		}

		/// @brief Get a reference to the value held by the Variant if it is of
		/// the specified type.
		/// @tparam I The index of the type to get in the Variant.
		/// @return An Option containing a reference to the value if the Variant
		/// holds a value of the specified type, or an empty Option if it does
		/// not.
		template<int I>
		auto get() -> Option<typename detail::VariantGet<I,
		    detail::VariantStorage<Ts...>>::Type &>
		{
			return get_impl<I>(*this);
		}
		/// @brief Get a reference to the value held by the Variant if it is of
		/// the specified type.
		/// @tparam I The index of the type to get in the Variant.
		/// @return An Option containing a reference to the value if the Variant
		/// holds a value of the specified type, or an empty Option if it does
		/// not.
		template<int I>
		auto get() const -> Option<typename detail::VariantGet<I,
		    detail::VariantStorage<Ts...>>::Type const &>
		{
			return get_impl<I>(*this);
		}

		/// @brief Get a reference to the value held by the Variant if it is of
		/// the specified type.
		/// @tparam T The type to get from the Variant.
		/// @return An Option containing a reference to the value if the Variant
		/// holds a value of the specified type, or an empty Option if it does
		/// not.
		template<typename T> auto get() -> Option<T &>
		{
			constexpr int I = detail::VariantIndex<T, Ts...>::value;
			return get<I>();
		}
		/// @brief Get a reference to the value held by the Variant if it is of
		/// the specified type.
		/// @tparam T The type to get from the Variant.
		/// @return An Option containing a reference to the value if the Variant
		/// holds a value of the specified type, or an empty Option if it does
		/// not.
		template<typename T> auto get() const -> Option<T const &>
		{
			constexpr int I = detail::VariantIndex<T, Ts...>::value;
			return get<I>();
		}

		/// @brief Apply a visitor to the value held by the Variant.
		/// @tparam Visitor The type of the visitor.
		/// @param visitor The visitor to apply.
		/// @return The result of applying the visitor.
		template<typename Visitor>
		auto visit(Visitor &&visitor) -> decltype(auto)
		{
			return visit_impl(*this, forward<Visitor>(visitor));
		}
		/// @brief Apply a visitor to the value held by the Variant.
		/// @tparam Visitor The type of the visitor.
		/// @param visitor The visitor to apply.
		/// @return The result of applying the visitor.
		template<typename Visitor>
		auto visit(Visitor &&visitor) const -> decltype(auto)
		{
			return visit_impl(*this, forward<Visitor>(visitor));
		}

		/// @brief Apply a function to the value held by the Variant if it is of
		/// the specified type.
		/// @tparam Fn The type of the function.
		/// @param fn The function to apply.
		/// @return The result of applying the function.
		template<typename Fn> auto map(Fn &&fn)
		{
			return visit([&](auto &value) { return fn(value); });
		}

		/// @brief Apply a function to the value held by the Variant if it is of
		/// the specified type.
		/// @tparam Fn The type of the function.
		/// @param fn The function to apply.
		/// @return The result of applying the function.
		template<typename Fn> auto and_then(Fn &&fn)
		{
			return visit([&](auto &value) { return fn(value); });
		}

		/// @brief Apply a function to the value held by the Variant if it is of
		/// the specified type.
		/// @tparam Fn The type of the function.
		/// @param fn The function to apply.
		/// @return A reference to the Variant after applying the function.
		template<typename Fn> auto inspect(Fn &&fn) -> Variant &
		{
			visit([&](auto &value) { fn(value); });
			return *this;
		}

		/// @brief Check if the Variant currently holds a value of the specified
		/// type.
		/// @tparam T The type to check for.
		/// @return true if the Variant holds T, false otherwise.
		template<typename T> auto is() const -> bool
		{
			constexpr int I = detail::VariantIndex<T, Ts...>::value;
			return m_tag == I;
		}

		/// @brief Check if the Variant currently holds the type at the
		/// specified index.
		/// @tparam I The index to check for.
		/// @return true if the Variant holds the type at index I, false
		/// otherwise.
		template<int I> auto is() const -> bool { return m_tag == I; }

		/// @brief Get the index of the type currently held by the Variant.
		/// @return The index of the type currently held by the Variant.
		auto tag() const -> int { return m_tag; }

	private:
		Variant() = default;

		template<typename Other> auto copy_from(Other const &other) -> void
		{
			other.visit([&](auto const &value) {
				using T = RemoveConstRef<decltype(value)>;
				constexpr int I = detail::VariantIndex<T, Ts...>::value;
				construct<I>(value);
			});
		}

		template<typename Other> auto move_from(Other &other) -> void
		{
			other.visit([&](auto &value) {
				using T = RemoveConstRef<decltype(value)>;
				constexpr int I = detail::VariantIndex<T, Ts...>::value;
				construct<I>(move(value));
			});
		}

		template<int I, typename Self>
		static auto get_impl(Self &&self) -> decltype(auto)
		{
			if (self.m_tag != I)
				return Option<decltype(detail::VariantGet<I,
				    detail::VariantStorage<Ts...>>::get(self.m_data))> {};

			return Option<decltype(detail::VariantGet<I,
			    detail::VariantStorage<Ts...>>::get(self.m_data))>(
			    detail::VariantGet<I, detail::VariantStorage<Ts...>>::get(
			        self.m_data));
		}

		template<int I, typename... Args> auto construct(Args &&...args) -> void
		{
			using T = typename detail::VariantGet<I,
			    detail::VariantStorage<Ts...>>::Type;

			m_tag = I;
			new (&detail::VariantGet<I, detail::VariantStorage<Ts...>>::get(
			    m_data)) T(forward<Args>(args)...);
		}

		auto destroy() -> void { destroy_impl(); }

		template<int I = 0> auto destroy_impl() -> void
		{
			if constexpr (I < sizeof...(Ts)) {
				if (m_tag == I) {
					using T = typename detail::VariantGet<I,
					    detail::VariantStorage<Ts...>>::Type;

					detail::VariantGet<I, detail::VariantStorage<Ts...>>::get(
					    m_data)
					    .~T();
				} else {
					destroy_impl<I + 1>();
				}
			}
		}

		template<int I = 0, typename Self, typename Visitor>
		static decltype(auto) visit_impl(Self &&self, Visitor &&visitor)
		{
			if constexpr (I >= sizeof...(Ts)) {
				__builtin_unreachable();
			} else {
				if (self.m_tag == I) {
					return forward<Visitor>(visitor)(detail::VariantGet<I,
					    detail::VariantStorage<Ts...>>::get(self.m_data));
				}

				return visit_impl<I + 1>(
				    forward<Self>(self), forward<Visitor>(visitor));
			}
		}

	private:
		int m_tag {};
		detail::VariantStorage<Ts...> m_data;
	};

	}
}
