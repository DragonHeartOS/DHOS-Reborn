#pragma once

#include <CommonLib/Types.h>
#include <CommonLib/cstring>

namespace CL {

template<typename CharTypeT> struct BaseStringView {
	BaseStringView(CharTypeT const *c_str)
	    : m_data(c_str)
	    , m_size(strlen(c_str))
	{
	}

	auto data() -> CharTypeT const * { return m_data; }
	auto data() const -> CharTypeT const * { return m_data; }

	auto size() -> usize { return m_size; }
	auto size() const -> usize { return m_size; }

private:
	CharTypeT const *m_data { nullptr };
	usize m_size {};
};

using StringView = BaseStringView<char>;

}
