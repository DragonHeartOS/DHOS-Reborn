#pragma once

#include <CommonLib/ArrayList.h>
#include <CommonLib/StringView.h>
#include <CommonLib/Types.h>

namespace CL {

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

	auto iter() -> Iter
	{
		return Iter {
			.iter = m_data.iter(),
			.remaining = size(),
		};
	}

	auto iter() const -> ConstIter
	{
		return ConstIter {
			.iter = m_data.iter(),
			.remaining = size(),
		};
	}

	constexpr auto size() const -> usize
	{
		return m_data.size() ? m_data.size() - 1 : 0;
	}

	constexpr auto is_empty() const -> bool { return size() == 0; }

	constexpr auto data() -> CharTypeT * { return m_data.data(); }
	constexpr auto data() const -> CharTypeT const * { return m_data.data(); }

	constexpr auto c_str() const -> CharTypeT const * { return m_data.data(); }

	constexpr auto view() const -> BaseStringView<CharTypeT>
	{
		return { data(), size() };
	}

	auto clear() -> void
	{
		m_data.clear();
		m_data.push('\0');
	}

	auto append(CharTypeT ch) -> void
	{
		m_data.pop();
		m_data.push(ch);
		m_data.push('\0');
	}

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

	constexpr auto starts_with(BaseStringView<CharTypeT> const &prefix) const
	    -> bool
	{
		if (prefix.size() > size())
			return false;

		for (usize i {}; i < prefix.size(); ++i) {
			if (data()[i] != prefix.data()[i])
				return false;
		}

		return true;
	}

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

	constexpr auto find(BaseStringView<CharTypeT> const &needle) const -> usize
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

	constexpr auto contains(BaseStringView<CharTypeT> const &needle) const
	    -> bool
	{
		return find(needle) != npos;
	}

	auto substring(usize start, usize count) const -> BaseString
	{
		BaseString result;

		if (start >= size())
			return result;

		if (start + count > size())
			count = size() - start;

		result.m_data.pop();
		result.m_data.reserve(count + 1);

		for (usize i {}; i < count; ++i)
			result.m_data.push(data()[start + i]);

		result.m_data.push('\0');
		return result;
	}

	constexpr auto operator==(BaseStringView<CharTypeT> const &other) const
	    -> bool
	{
		if (size() != other.size())
			return false;

		for (usize i {}; i < size(); ++i) {
			if (data()[i] != other.data()[i])
				return false;
		}

		return true;
	}

	constexpr auto operator!=(BaseStringView<CharTypeT> const &other) const
	    -> bool
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

inline auto to_display_string(String const &value) -> String { return value; }
inline auto to_display_string(StringView const value) -> String
{
	return String(value);
}
inline auto to_display_string(char const *value) -> String { return String(value); }

inline auto to_debug_string(String const &value) -> String { return value; }
inline auto to_debug_string(StringView const value) -> String
{
	return String(value);
}
inline auto to_debug_string(char const *value) -> String { return String(value); }

}

}
