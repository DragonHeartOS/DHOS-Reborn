#pragma once

#include <CommonLib/InitializerList.h>
#include <CommonLib/Types.h>
#include <CommonLib/cstring>

namespace CL {

template<typename T> struct Span {
	explicit Span(T *data, usize size)
	    : m_data { data }
	    , m_size { size }
	{
	}

	explicit Span(InitializerList<T> init)
	{
		m_size = init.size();
		m_data = new T[m_size];
		memcpy(m_data, init.data(), m_size * sizeof(T));
	}

private:
	T *m_data;
	usize m_size;
};

}
