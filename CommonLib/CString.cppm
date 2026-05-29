export module CommonLib:CString;

import :Types;

export {
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

	extern "C" auto memcpy(void *dst, void *src, usize size) -> void *
	{
		auto *dst_ { reinterpret_cast<u8 *>(dst) };
		auto *src_ { reinterpret_cast<u8 *>(src) };
		for (usize i {}; i < size; i++)
			dst_[i] = src_[i];
		return dst;
	}

	extern "C" auto memset(void *dst, int value, usize size) -> void *
	{
		auto *dst_ { reinterpret_cast<u8 *>(dst) };
		for (usize i {}; i < size; i++)
			dst_[i] = static_cast<u8>(value);
		return dst;
	}
}
