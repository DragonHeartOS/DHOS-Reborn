export module CommonLib:StringView;

import :Types;

export {
	namespace CL {

	/// @brief A non-owning view of a string of characters.
	/// @tparam CharTypeT The type of the characters in the string view.
	template<typename CharTypeT> struct BaseStringView {
		static constexpr auto length(CharTypeT const *c_str) -> usize
		{
			usize len {};
			while (c_str[len] != '\0')
				len++;

			return len;
		}

		constexpr BaseStringView(CharTypeT const *c_str)
		    : m_data { c_str }
		    , m_size { length(c_str) }
		{
		}

		constexpr BaseStringView(CharTypeT const *data, usize size)
		    : m_data { data }
		    , m_size { size }
		{
		}

		/// @brief Get a pointer to the characters in the string view.
		/// @return A pointer to the characters in the string view.
		constexpr auto data() -> CharTypeT const * { return m_data; }
		/// @brief Get a pointer to the characters in the string view.
		/// @return A pointer to the characters in the string view.
		constexpr auto data() const -> CharTypeT const * { return m_data; }

		/// @brief Get the size of the string view.
		/// @return The size of the string view.
		constexpr auto size() -> usize { return m_size; }
		/// @brief Get the size of the string view.
		/// @return The size of the string view.
		constexpr auto size() const -> usize { return m_size; }

		// /// @brief Get a span over the string view.
		// /// @return A span over the string view.
		// constexpr auto span() const -> Span<CharTypeT const>
		// {
		// 	return Span<CharTypeT const>(data(), size());
		// }

		// /// @brief Get a span over the first characters of the string view.
		// /// @param length The number of characters to include.
		// /// @return A span over the requested prefix.
		// constexpr auto span(usize length) const -> Span<CharTypeT const>
		// {
		// 	return span(0, length);
		// }

		// /// @brief Get a span over a subrange of the string view.
		// /// @param start The starting index of the span.
		// /// @param length The number of characters to include.
		// /// @return A span over the requested subrange.
		// constexpr auto span(usize start, usize length) const
		//     -> Span<CharTypeT const>
		// {
		// 	if (start >= size())
		// 		return Span<CharTypeT const>(data(), 0);

		// 	if (start + length > size())
		// 		length = size() - start;

		// 	return Span<CharTypeT const>(data() + start, length);
		// }

		constexpr auto operator==(BaseStringView const &other) const -> bool
		{
			if (size() != other.size())
				return false;

			for (usize i {}; i < size(); ++i) {
				if (data()[i] != other.data()[i])
					return false;
			}

			return true;
		}

		constexpr auto operator!=(BaseStringView const &other) const -> bool
		{
			return !(*this == other);
		}

	private:
		CharTypeT const *m_data { nullptr };
		usize m_size {};
	};

	using StringView = BaseStringView<char>;

	}
}
