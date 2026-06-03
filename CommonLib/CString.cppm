export module CommonLib:CString;

import :Types;

export {
	namespace CL {

	auto strlen(char const *str) -> usize
	{
		char const *s;
		for (s = str; *s; ++s)
			;
		return static_cast<usize>(s - str);
	}

	auto strcpy(char *destination, char const *source) -> char *
	{
		if (destination == nullptr)
			return nullptr;

		char *ptr = destination;

		while (*source != '\0') {
			*destination = *source;
			destination++;
			source++;
		}

		*destination = '\0';
		return ptr;
	}

	auto revstr(char *str1) -> void
	{
		int i, len, temp;
		len = (int)strlen(str1);
		for (i = 0; i < len / 2; i++) {
			temp = str1[i];
			str1[i] = str1[len - i - 1];
			str1[len - i - 1] = (char)temp;
		}
	}

	auto itoa(int n, char s[]) -> void
	{
		int i, sign;

		if ((sign = n) < 0)
			n = -n;
		i = 0;
		do {
			s[i++] = n % 10 + '0';
		} while ((n /= 10) > 0);
		if (sign < 0)
			s[i++] = '-';
		s[i] = '\0';
		revstr(s);
	}

	extern "C" auto memcpy(void *dst, void const *src, usize size) -> void *
	{
		auto *d = reinterpret_cast<u8 *>(dst);
		auto const *s = reinterpret_cast<u8 const *>(src);

		usize qwords = size / 8;
		usize bytes = size % 8;

		asm volatile("cld; rep movsq"
		    : "+D"(d), "+S"(s), "+c"(qwords)
		    :
		    : "memory");

		asm volatile("rep movsb" : "+D"(d), "+S"(s), "+c"(bytes) : : "memory");

		return dst;
	}

	extern "C" auto memmove(void *dst, void const *src, usize size) -> void *
	{
		auto *d = reinterpret_cast<u8 *>(dst);
		auto const *s = reinterpret_cast<u8 const *>(src);

		if (!size || d == s)
			return dst;

		if (d < s || d >= s + size)
			return memcpy(dst, src, size);

		d += size - 1;
		s += size - 1;

		asm volatile("std; rep movsb; cld"
		    : "+D"(d), "+S"(s), "+c"(size)
		    :
		    : "memory");

		return dst;
	}

	extern "C" auto memset(void *dst, int value, usize size) -> void *
	{
		auto *d = reinterpret_cast<u8 *>(dst);
		u8 b = static_cast<u8>(value);

		u64 q = 0x0101010101010101ull * b;

		usize qwords = size / 8;
		usize bytes = size % 8;

		asm volatile("cld; rep stosq"
		    : "+D"(d), "+c"(qwords)
		    : "a"(q)
		    : "memory");

		asm volatile("rep stosb" : "+D"(d), "+c"(bytes) : "a"(b) : "memory");

		return dst;
	}

	}
}
