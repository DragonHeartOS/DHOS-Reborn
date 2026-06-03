export module CommonLib:String;

import :ArrayList;
import :Span;
import :StringView;
import :Types;

export {
	namespace CL {

	/// @brief An owning string of characters.
	/// @tparam CharTypeT The type of the characters in the string.
	template<typename CharTypeT> struct BaseString {
		struct Iter : Iterator<Iter> {
			ArrayList<CharTypeT>::Iter iter;
			usize remaining;

			auto next() -> Option<CharTypeT &>
			{
				if (remaining == 0)
					return {};

				remaining--;
				return iter.next();
			}

			auto next_back() -> Option<CharTypeT &>
			{
				if (remaining == 0)
					return {};

				remaining--;
				return iter.next_back();
			}
		};

		struct ConstIter : Iterator<ConstIter> {
			ArrayList<CharTypeT>::ConstIter iter;
			usize remaining;

			auto next() -> Option<CharTypeT const &>
			{
				if (remaining == 0)
					return {};

				remaining--;
				return iter.next();
			}

			auto next_back() -> Option<CharTypeT const &>
			{
				if (remaining == 0)
					return {};

				remaining--;
				return iter.next_back();
			}
		};

		BaseString() { m_data.push('\0'); }

		BaseString(CharTypeT const *cstring)
		{
			usize len {};
			while (cstring[len] != '\0')
				len++;

			m_data.reserve(len + 1);

			for (usize i {}; i < len; ++i)
				m_data.push(cstring[i]);

			m_data.push('\0');
		}

		explicit BaseString(BaseStringView<CharTypeT> const &string)
		{
			m_data.reserve(string.size() + 1);

			for (usize i {}; i < string.size(); ++i)
				m_data.push(string.data()[i]);

			m_data.push('\0');
		}

		/// @brief Get an iterator over the characters in the string.
		/// @return An iterator over the characters in the string.
		auto iter() -> Iter
		{
			return Iter {
				.iter = m_data.iter(),
				.remaining = size(),
			};
		}

		/// @brief Get an iterator over the characters in the string.
		/// @return An iterator over the characters in the string.
		auto iter() const -> ConstIter
		{
			return ConstIter {
				.iter = m_data.iter(),
				.remaining = size(),
			};
		}

		/// @brief Get the size of the string.
		/// @return The size of the string.
		constexpr auto size() const -> usize
		{
			return m_data.size() ? m_data.size() - 1 : 0;
		}

		/// @brief Get the current capacity of the string buffer.
		/// @return The number of characters that can be stored without
		/// reallocation.
		constexpr auto capacity() const -> usize
		{
			usize buffer_capacity = m_data.capacity();
			return buffer_capacity ? buffer_capacity - 1 : 0;
		}

		/// @brief Check if the string is empty.
		/// @return True if the string is empty, false otherwise.
		constexpr auto is_empty() const -> bool { return size() == 0; }

		/// @brief Get a pointer to the characters in the string.
		/// @return A pointer to the characters in the string.
		constexpr auto data() -> CharTypeT * { return m_data.data(); }
		/// @brief Get a pointer to the characters in the string.
		/// @return A pointer to the characters in the string.
		constexpr auto data() const -> CharTypeT const *
		{
			return m_data.data();
		}

		/// @brief Get a null-terminated C string.
		/// @return A null-terminated C string.
		constexpr auto c_str() const -> CharTypeT const *
		{
			return m_data.data();
		}

		/// @brief Get a string view of the string.
		/// @return A string view of the string.
		constexpr auto view() const -> BaseStringView<CharTypeT>
		{
			return { data(), size() };
		}

		/// @brief Get a span over the string contents.
		/// @return A span over the string contents.
		constexpr auto span() -> Span<CharTypeT>
		{
			return Span<CharTypeT>(data(), size());
		}

		/// @brief Get a span over the string contents.
		/// @return A span over the string contents.
		constexpr auto span() const -> Span<CharTypeT const>
		{
			return Span<CharTypeT const>(data(), size());
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
			if (start >= size())
				return Span<CharTypeT>(data(), 0);

			if (start + length > size())
				length = size() - start;

			return Span<CharTypeT>(data() + start, length);
		}

		/// @brief Get a span over a subrange of the string.
		/// @param start The starting index of the span.
		/// @param length The number of characters to include.
		/// @return A span over the requested subrange.
		constexpr auto span(usize start, usize length) const
		    -> Span<CharTypeT const>
		{
			if (start >= size())
				return Span<CharTypeT const>(data(), 0);

			if (start + length > size())
				length = size() - start;

			return Span<CharTypeT const>(data() + start, length);
		}

		/// @brief Clear the string, making it empty.
		auto clear() -> void
		{
			m_data.clear();
			m_data.push('\0');
		}

		/// @brief Append a character to the end of the string.
		/// @param ch The character to append.
		auto append(CharTypeT ch) -> void
		{
			m_data.pop();
			m_data.push(ch);
			m_data.push('\0');
		}

		/// @brief Append a string to the end of the string.
		/// @param string The string to append.
		auto append(BaseStringView<CharTypeT> const &string) -> void
		{
			m_data.pop();
			m_data.reserve(size() + string.size() + 1);

			for (usize i {}; i < string.size(); ++i)
				m_data.push(string.data()[i]);

			m_data.push('\0');
		}

		auto operator+=(CharTypeT ch) -> BaseString &
		{
			append(ch);
			return *this;
		}

		auto operator+=(BaseStringView<CharTypeT> const &string) -> BaseString &
		{
			append(string);
			return *this;
		}

		constexpr auto operator[](usize index) -> CharTypeT &
		{
			return m_data[index];
		}

		constexpr auto operator[](usize index) const -> CharTypeT const &
		{
			return m_data[index];
		}

		/// @brief Check if the string starts with the specified prefix.
		/// @param prefix The prefix to check for.
		/// @return True if the string starts with the specified prefix, false
		/// otherwise.
		constexpr auto starts_with(
		    BaseStringView<CharTypeT> const &prefix) const -> bool
		{
			if (prefix.size() > size())
				return false;

			for (usize i {}; i < prefix.size(); ++i) {
				if (data()[i] != prefix.data()[i])
					return false;
			}

			return true;
		}

		/// @brief Check if the string ends with the specified suffix.
		/// @param suffix The suffix to check for.
		/// @return True if the string ends with the specified suffix, false
		/// otherwise.
		constexpr auto ends_with(BaseStringView<CharTypeT> const &suffix) const
		    -> bool
		{
			if (suffix.size() > size())
				return false;

			usize offset = size() - suffix.size();

			for (usize i {}; i < suffix.size(); ++i) {
				if (data()[offset + i] != suffix.data()[i])
					return false;
			}

			return true;
		}

		/// @brief Find the first occurrence of the specified substring in the
		/// string.
		/// @param needle The substring to find.
		/// @return The index of the first occurrence of the substring in the
		/// string, or npos if the substring was not found.
		constexpr auto find(BaseStringView<CharTypeT> const &needle) const
		    -> usize
		{
			if (needle.size() == 0)
				return 0;

			if (needle.size() > size())
				return npos;

			for (usize i {}; i <= size() - needle.size(); ++i) {
				bool found = true;

				for (usize j {}; j < needle.size(); ++j) {
					if (data()[i + j] != needle.data()[j]) {
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
		constexpr auto contains(BaseStringView<CharTypeT> const &needle) const
		    -> bool
		{
			return find(needle) != npos;
		}

		/// @brief Get a substring view of the string.
		/// @param start The starting index of the substring.
		/// @param count The number of characters in the substring.
		/// @return A BaseStringView into the string.
		auto substring(usize start, usize count) const
		    -> BaseStringView<CharTypeT>
		{
			if (start >= size())
				return {};

			if (start + count > size())
				count = size() - start;

			return BaseStringView(data() + start, count);
		}

		/// @brief Get a substring view of the string.
		/// @param start The starting index of the substring.
		/// @return A BaseStringView into the string.
		auto substring(usize start) const -> BaseStringView<CharTypeT>
		{
			if (start >= size())
				return {};

			return BaseStringView(data() + start, size() - start);
		}

		constexpr auto operator==(BaseStringView<CharTypeT> const &other) const
		    -> bool
		{
			return view() == other;
		}

		constexpr auto operator==(BaseString const &other) const -> bool
		{
			return view() == other.view();
		}

		constexpr auto operator!=(BaseStringView<CharTypeT> const &other) const
		    -> bool
		{
			return !(*this == other);
		}

		constexpr auto operator!=(BaseString const &other) const -> bool
		{
			return !(*this == other);
		}

		static constexpr usize npos = static_cast<usize>(-1);

	private:
		ArrayList<CharTypeT> m_data;
	};

	template<typename CharTypeT>
	auto operator+(BaseString<CharTypeT> const &lhs,
	    BaseStringView<CharTypeT> const &rhs) -> BaseString<CharTypeT>
	{
		BaseString<CharTypeT> result(lhs.view());
		result += rhs;
		return result;
	}

	using String = BaseString<char>;

	namespace detail::adl {

	constexpr auto hash_string(StringView const value) -> usize
	{
		usize hash { 14695981039346656037ull };

		for (usize i {}; i < value.size(); ++i) {
			hash ^= static_cast<unsigned char>(value.data()[i]);
			hash *= 1099511628211ull;
		}

		return hash;
	}

	inline auto to_display_string(String const &value) -> String
	{
		return value;
	}
	inline auto to_display_string(StringView const value) -> String
	{
		return String(value);
	}
	inline auto to_display_string(char const *value) -> String
	{
		return String(value);
	}

	inline auto to_debug_string(String const &value) -> String { return value; }
	inline auto to_debug_string(StringView const value) -> String
	{
		return String(value);
	}
	inline auto to_debug_string(char const *value) -> String
	{
		return String(value);
	}

	inline auto to_hash(String const &value) -> usize
	{
		return hash_string(value.view());
	}
	inline auto to_hash(StringView const value) -> usize
	{
		return hash_string(value);
	}
	inline auto to_hash(char const *value) -> usize
	{
		return hash_string(StringView(value));
	}

	}

	}
}
