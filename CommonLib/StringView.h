#pragma once

#include <CommonLib/Types.h>

namespace CL {

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

	constexpr auto data() -> CharTypeT const * { return m_data; }
	constexpr auto data() const -> CharTypeT const * { return m_data; }

	constexpr auto size() -> usize { return m_size; }
	constexpr auto size() const -> usize { return m_size; }

private:
	CharTypeT const *m_data { nullptr };
	usize m_size {};
};

using StringView = BaseStringView<char>;

}
