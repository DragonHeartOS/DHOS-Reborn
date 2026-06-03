module;

#include <string.h>

export module LibC:String;

import CommonLib;

export {

	extern "C" [[gnu::used]] size_t strlen(char const *str)
	{
		return CL::strlen(str);
	}
	extern "C" [[gnu::used]] void *memcpy(
	    void *dst, void const *src, size_t size)
	{
		return CL::memcpy(dst, src, size);
	}
	extern "C" [[gnu::used]] void *memmove(
	    void *dst, void const *src, size_t size)
	{
		return CL::memmove(dst, src, size);
	}
	extern "C" [[gnu::used]] void *memset(void *dst, int value, size_t size)
	{
		return CL::memset(dst, value, size);
	}
}
