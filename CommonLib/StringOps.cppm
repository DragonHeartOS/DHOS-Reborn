export module CommonLib:StringOps;

import :Option;
import :Span;
import :Types;

export {
	namespace CL {

	template<typename CharTypeT> struct BaseStringView;

	namespace detail {

	template<typename T>
	concept StringLike = requires(T const &value) {
		value.data();
		value.size();
	};

	template<class Derived, typename CharTypeT> struct StringViewOps {
		constexpr auto self() -> Derived &
		{
			return static_cast<Derived &>(*this);
		}
		constexpr auto self() const -> Derived const &
		{
			return static_cast<Derived const &>(*this);
		}

		/// @brief Check if the string is empty.
		/// @return True if the string is empty, false otherwise.
		constexpr auto is_empty() const -> bool { return self().size() == 0; }

		/// @brief Get a string view of the string.
		/// @return A string view of the string.
		constexpr auto view() const -> BaseStringView<CharTypeT>
		{
			return { self().data(), self().size() };
		}

		/// @brief Check if the string starts with the specified prefix.
		/// @param prefix The prefix to check for.
		/// @return True if the string starts with the specified prefix, false
		/// otherwise.
		template<StringLike Other>
		constexpr auto starts_with(Other const &prefix) const -> bool
		{
			if (prefix.size() > self().size())
				return false;

			for (usize i {}; i < prefix.size(); ++i) {
				if (self().data()[i] != prefix.data()[i])
					return false;
			}

			return true;
		}

		/// @brief Check if the string ends with the specified suffix.
		/// @param suffix The suffix to check for.
		/// @return True if the string ends with the specified suffix, false
		/// otherwise.
		template<StringLike Other>
		constexpr auto ends_with(Other const &suffix) const -> bool
		{
			if (suffix.size() > self().size())
				return false;

			usize offset = self().size() - suffix.size();

			for (usize i {}; i < suffix.size(); ++i) {
				if (self().data()[offset + i] != suffix.data()[i])
					return false;
			}

			return true;
		}

		/// @brief Find the first occurrence of the specified substring in the
		/// string.
		/// @param needle The substring to find.
		/// @return The index of the first occurrence of the substring in the
		/// string, or npos if the substring was not found.
		template<StringLike Other>
		constexpr auto find(Other const &needle) const -> usize
		{
			if (needle.size() == 0)
				return 0;

			if (needle.size() > self().size())
				return npos;

			for (usize i {}; i <= self().size() - needle.size(); ++i) {
				bool found = true;

				for (usize j {}; j < needle.size(); ++j) {
					if (self().data()[i + j] != needle.data()[j]) {
						found = false;
						break;
					}
				}

				if (found)
					return i;
			}

			return npos;
		}

		/// @brief Check if the string contains the specified substring.
		/// @param needle The substring to check for.
		/// @return True if the string contains the specified substring, false
		/// otherwise.
		template<StringLike Other>
		constexpr auto contains(Other const &needle) const -> bool
		{
			return find(needle) != npos;
		}

		/// @brief Get a substring view of the string.
		/// @param start The starting index of the substring.
		/// @param count The number of characters in the substring.
		/// @return A BaseStringView into the string.
		constexpr auto substring(usize start, usize count) const
		    -> BaseStringView<CharTypeT>
		{
			if (start >= self().size())
				return {};

			if (start + count > self().size())
				count = self().size() - start;

			return BaseStringView<CharTypeT>(self().data() + start, count);
		}

		/// @brief Get a substring view of the string.
		/// @param start The starting index of the substring.
		/// @return A BaseStringView into the string.
		constexpr auto substring(usize start) const -> BaseStringView<CharTypeT>
		{
			if (start >= self().size())
				return {};

			return BaseStringView<CharTypeT>(
			    self().data() + start, self().size() - start);
		}

		/// @brief Check if the string is equal to another string-like value.
		/// @param other The string-like value to compare against.
		/// @return True if the strings are equal, false otherwise.
		template<StringLike Other>
		constexpr auto operator==(Other const &other) const -> bool
		{
			if (self().size() != other.size())
				return false;

			for (usize i {}; i < self().size(); ++i) {
				if (self().data()[i] != other.data()[i])
					return false;
			}

			return true;
		}

		/// @brief Check if the string is not equal to another string-like
		/// value.
		/// @param other The string-like value to compare against.
		/// @return True if the strings are not equal, false otherwise.
		template<StringLike Other>
		constexpr auto operator!=(Other const &other) const -> bool
		{
			return !(*this == other);
		}

		static constexpr usize npos = static_cast<usize>(-1);
	};

	template<class Derived, typename CharTypeT>
	struct StringOps : StringViewOps<Derived, CharTypeT> {
		/// @brief Get a span over the string contents.
		/// @return A span over the string contents.
		constexpr auto span() -> Span<CharTypeT>
		{
			return Span<CharTypeT>(this->self().data(), this->self().size());
		}

		/// @brief Get a span over the string contents.
		/// @return A span over the string contents.
		constexpr auto span() const -> Span<CharTypeT const>
		{
			return Span<CharTypeT const>(
			    this->self().data(), this->self().size());
		}

		/// @brief Get a span over the first characters of the string.
		/// @param length The number of characters to include.
		/// @return A span over the requested prefix.
		constexpr auto span(usize length) -> Span<CharTypeT>
		{
			return span(0, length);
		}

		/// @brief Get a span over the first characters of the string.
		/// @param length The number of characters to include.
		/// @return A span over the requested prefix.
		constexpr auto span(usize length) const -> Span<CharTypeT const>
		{
			return span(0, length);
		}

		/// @brief Get a span over a subrange of the string.
		/// @param start The starting index of the span.
		/// @param length The number of characters to include.
		/// @return A span over the requested subrange.
		constexpr auto span(usize start, usize length) -> Span<CharTypeT>
		{
			if (start >= this->self().size())
				return Span<CharTypeT>(this->self().data(), 0);

			if (start + length > this->self().size())
				length = this->self().size() - start;

			return Span<CharTypeT>(this->self().data() + start, length);
		}

		/// @brief Get a span over a subrange of the string.
		/// @param start The starting index of the span.
		/// @param length The number of characters to include.
		/// @return A span over the requested subrange.
		constexpr auto span(usize start, usize length) const
		    -> Span<CharTypeT const>
		{
			if (start >= this->self().size())
				return Span<CharTypeT const>(this->self().data(), 0);

			if (start + length > this->self().size())
				length = this->self().size() - start;

			return Span<CharTypeT const>(this->self().data() + start, length);
		}
	};

	} // namespace detail

	}
}
