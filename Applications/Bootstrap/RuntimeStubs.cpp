void *operator new(unsigned long size)
{
	(void)size;
	return nullptr;
}

void *operator new[](unsigned long size)
{
	(void)size;
	return nullptr;
}

void operator delete(void *ptr) noexcept
{
	(void)ptr;
}

void operator delete[](void *ptr) noexcept
{
	(void)ptr;
}

extern "C" auto memset(void *dst, int value, unsigned long size) -> void *
{
	auto *bytes = static_cast<unsigned char *>(dst);
	unsigned char const byte = static_cast<unsigned char>(value);

	for (unsigned long i {}; i < size; ++i)
		bytes[i] = byte;

	return dst;
}
