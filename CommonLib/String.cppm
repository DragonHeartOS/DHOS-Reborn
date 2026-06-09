export module CommonLib:String;

import :ArrayList;
import :Iterator;
import :Option;
import :StringOps;
import :StringView;
import :Types;

export {
	namespace CL {

	/// @brief An owning string of characters.
	/// @tparam CharTypeT The type of the characters in the string.
	template<typename CharTypeT>
	struct BaseString : detail::StringOps<BaseString<CharTypeT>, CharTypeT> {
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
