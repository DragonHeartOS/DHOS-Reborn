#include <new>

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

void *operator new(unsigned long size, std::nothrow_t const &) noexcept
{
	(void)size;
	return nullptr;
}

void *operator new[](unsigned long size, std::nothrow_t const &) noexcept
{
	(void)size;
	return nullptr;
}

void operator delete(void *ptr) noexcept { (void)ptr; }

void operator delete[](void *ptr) noexcept { (void)ptr; }

void operator delete(void *ptr, std::nothrow_t const &) noexcept { (void)ptr; }

void operator delete[](void *ptr, std::nothrow_t const &) noexcept
{
	(void)ptr;
}
