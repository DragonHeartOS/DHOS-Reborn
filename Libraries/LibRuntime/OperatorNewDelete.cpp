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
