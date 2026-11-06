extern "C" auto memset(void *dst, int value, unsigned long size) -> void *
{
	auto *bytes = static_cast<unsigned char *>(dst);
	auto const byte = static_cast<unsigned char>(value);

	for (unsigned long i {}; i < size; ++i)
		bytes[i] = byte;

	return dst;
}

extern "C" auto memcpy(void *dst, void const *src, unsigned long size) -> void *
{
	auto *d = static_cast<unsigned char *>(dst);
	auto const *s = static_cast<unsigned char const *>(src);

	for (unsigned long i {}; i < size; ++i)
		d[i] = s[i];

	return dst;
}

extern "C" auto memmove(void *dst, void const *src, unsigned long size)
    -> void *
{
	auto *d = static_cast<unsigned char *>(dst);
	auto const *s = static_cast<unsigned char const *>(src);

	if (d == s || size == 0)
		return dst;

	if (d < s) {
		for (unsigned long i {}; i < size; ++i)
			d[i] = s[i];
	} else {
		for (unsigned long i = size; i-- > 0;)
			d[i] = s[i];
	}

	return dst;
}

extern "C" auto memcmp(void const *lhs, void const *rhs, unsigned long size)
    -> int
{
	auto const *a = static_cast<unsigned char const *>(lhs);
	auto const *b = static_cast<unsigned char const *>(rhs);

	for (unsigned long i {}; i < size; ++i) {
		if (a[i] != b[i])
			return static_cast<int>(a[i]) - static_cast<int>(b[i]);
	}

	return 0;
}

extern "C" auto strlen(char const *str) -> unsigned long
{
	unsigned long len {};
	if (!str)
		return 0;
	while (str[len] != '\0')
		++len;
	return len;
}
