#pragma once

#include <CommonLib/Types.h>

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
	    : m_data(c_str)
	    , m_size(length(c_str))
	{
	}

	constexpr BaseStringView(CharTypeT const *data, usize size)
	    : m_data(data)
	    , m_size(size)
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
