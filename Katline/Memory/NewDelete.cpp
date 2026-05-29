
#include <stdint.h>

import CommonLib;
import Katline;

using Katline::Memory::MemoryManager;

[[noreturn]] static void allocation_panic()
{
	for (;;)
		asm("hlt");
}

auto operator new(usize size) -> void *
{
	if (size == 0)
		size = 1;

	if (auto *ptr = MemoryManager::allocate(size))
		return ptr;

	allocation_panic();
}

auto operator new[](usize size) -> void *
{
	if (size == 0)
		size = 1;

	if (auto *ptr = MemoryManager::allocate(size))
		return ptr;

	allocation_panic();
}

auto operator new(usize size, std::align_val_t alignment) noexcept -> void *
{
	if (size == 0)
		size = 1;

	if (auto *ptr = MemoryManager::allocate_aligned(size, alignment.value))
		return ptr;

	allocation_panic();
}

auto operator new[](usize size, std::align_val_t alignment) noexcept -> void *
{
	if (size == 0)
		size = 1;

	if (auto *ptr = MemoryManager::allocate_aligned(size, alignment.value))
		return ptr;

	allocation_panic();
}

auto operator new(usize size, std::nothrow_t const &) noexcept -> void *
{
	if (size == 0)
		size = 1;

	return MemoryManager::allocate(size);
}

auto operator new[](usize size, std::nothrow_t const &) noexcept -> void *
{
	if (size == 0)
		size = 1;

	return MemoryManager::allocate(size);
}

auto operator new(usize size, std::align_val_t alignment,
    std::nothrow_t const &) noexcept -> void *
{
	if (size == 0)
		size = 1;

	return MemoryManager::allocate_aligned(size, alignment.value);
}

auto operator new[](usize size, std::align_val_t alignment,
    std::nothrow_t const &) noexcept -> void *
{
	if (size == 0)
		size = 1;

	return MemoryManager::allocate_aligned(size, alignment.value);
}

auto operator delete(void *ptr) noexcept -> void { MemoryManager::free(ptr); }

auto operator delete[](void *ptr) noexcept -> void { MemoryManager::free(ptr); }

auto operator delete(void *ptr, usize) noexcept -> void
{
	MemoryManager::free(ptr);
}

auto operator delete[](void *ptr, usize) noexcept -> void
{
	MemoryManager::free(ptr);
}

auto operator delete(void *ptr, std::align_val_t) noexcept -> void
{
	MemoryManager::free(ptr);
}

auto operator delete[](void *ptr, std::align_val_t) noexcept -> void
{
	MemoryManager::free(ptr);
}

auto operator delete(void *ptr, usize, std::align_val_t) noexcept -> void
{
	MemoryManager::free(ptr);
}

auto operator delete[](void *ptr, usize, std::align_val_t) noexcept -> void
{
	MemoryManager::free(ptr);
}

auto operator delete(void *ptr, std::nothrow_t const &) noexcept -> void
{
	MemoryManager::free(ptr);
}

auto operator delete[](void *ptr, std::nothrow_t const &) noexcept -> void
{
	MemoryManager::free(ptr);
}

auto operator delete(
    void *ptr, std::align_val_t, std::nothrow_t const &) noexcept -> void
{
	MemoryManager::free(ptr);
}

auto operator delete[](
    void *ptr, std::align_val_t, std::nothrow_t const &) noexcept -> void
{
	MemoryManager::free(ptr);
}
