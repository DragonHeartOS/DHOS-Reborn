#include <Katline/Memory/MemoryManager.h>

#include <CommonLib/Types.h>
#include <Katline/Debug.h>
#include <Katline/Memory/Heap.h>
#include <Katline/Memory/MemoryData.h>

namespace Katline {

namespace Memory {

auto MemoryManager::init(MemoryMap const *mmap) -> void
{
	bool initialized {};

	for (u64 i {}; i < mmap->size; i++) {
		auto *md = &mmap->data[i];

		if (md->type != MemoryType::USABLE || md->size == 0)
			continue;

		if (!initialized) {
			mrvn_memory_init((void *)md->base, md->size);
			initialized = true;
			continue;
		}

		mrvn_memory_add((void *)md->base, md->size);
	}

	if (!initialized) {
		Debug::write_formatted(
		    "[MemoryManager] Failed to initialize: no usable memory region.\n");
		for (;;)
			asm("hlt");
	}

	Debug::write_formatted("[MemoryManager] Initialized.\n");
}

auto MemoryManager::allocate(usize const size) -> void *
{
	return mrvn_malloc(size);
}

struct AllocationHeader {
	void *raw;
	u64 magic;
};

constexpr u64 ALIGNED_MAGIC = 0xA7B04F1EULL;

auto MemoryManager::allocate_aligned(usize const size, usize alignment)
    -> void *
{
	if (alignment < sizeof(void *))
		alignment = sizeof(void *);

	if ((alignment & (alignment - 1)) != 0)
		return nullptr;

	usize const total { size + alignment - 1 + sizeof(AllocationHeader) };

	void *raw = mrvn_malloc(total);
	if (!raw)
		return nullptr;

	auto const start { reinterpret_cast<uintptr_t>(raw)
		+ sizeof(AllocationHeader) };
	uintptr_t const aligned { (start + alignment - 1) & ~(alignment - 1) };

	auto *header { reinterpret_cast<AllocationHeader *>(
		aligned - sizeof(AllocationHeader)) };
	header->raw = raw;
	header->magic = ALIGNED_MAGIC;

	return reinterpret_cast<void *>(aligned);
}

auto MemoryManager::free(void *ptr) -> void
{
	if (!ptr)
		return;

	auto *header {
		reinterpret_cast<AllocationHeader *>(
		    reinterpret_cast<uintptr_t>(ptr) - sizeof(AllocationHeader)),
	};

	if (header->magic == ALIGNED_MAGIC) {
		mrvn_free(header->raw);
		return;
	}

	mrvn_free(ptr);
}

}

}

namespace std {

using size_t = ::usize;

struct nothrow_t {
	explicit nothrow_t() = default;
};

struct align_val_t {
	explicit align_val_t(size_t a)
	    : value(a)
	{
	}

	size_t value;
};

inline constexpr nothrow_t nothrow {};

class bad_alloc { };

}

using Katline::Memory::MemoryManager;

[[noreturn]] static void allocation_panic()
{
	for (;;)
		asm("hlt");
}

auto operator new(std::size_t size) -> void *
{
	if (size == 0)
		size = 1;

	if (auto *ptr = MemoryManager::allocate(size))
		return ptr;

	allocation_panic();
}

auto operator new[](std::size_t size) -> void *
{
	if (size == 0)
		size = 1;

	if (auto *ptr = MemoryManager::allocate(size))
		return ptr;

	allocation_panic();
}

auto operator new(std::size_t size, std::align_val_t alignment) -> void *
{
	if (size == 0)
		size = 1;

	if (auto *ptr = MemoryManager::allocate_aligned(size, alignment.value))
		return ptr;

	allocation_panic();
}

auto operator new[](std::size_t size, std::align_val_t alignment) -> void *
{
	if (size == 0)
		size = 1;

	if (auto *ptr = MemoryManager::allocate_aligned(size, alignment.value))
		return ptr;

	allocation_panic();
}

auto operator new(std::size_t size, std::nothrow_t const &) noexcept -> void *
{
	if (size == 0)
		size = 1;

	return MemoryManager::allocate(size);
}

auto operator new[](std::size_t size, std::nothrow_t const &) noexcept -> void *
{
	if (size == 0)
		size = 1;

	return MemoryManager::allocate(size);
}

auto operator new(std::size_t size, std::align_val_t alignment,
    std::nothrow_t const &) noexcept -> void *
{
	if (size == 0)
		size = 1;

	return MemoryManager::allocate_aligned(size, alignment.value);
}

auto operator new[](std::size_t size, std::align_val_t alignment,
    std::nothrow_t const &) noexcept -> void *
{
	if (size == 0)
		size = 1;

	return MemoryManager::allocate_aligned(size, alignment.value);
}

auto operator delete(void *ptr) noexcept -> void { MemoryManager::free(ptr); }

auto operator delete[](void *ptr) noexcept -> void { MemoryManager::free(ptr); }

auto operator delete(void *ptr, std::size_t) noexcept -> void
{
	MemoryManager::free(ptr);
}

auto operator delete[](void *ptr, std::size_t) noexcept -> void
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

auto operator delete(void *ptr, std::size_t, std::align_val_t) noexcept -> void
{
	MemoryManager::free(ptr);
}

auto operator delete[](void *ptr, std::size_t, std::align_val_t) noexcept
    -> void
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

inline auto operator new(std::size_t, void *ptr) noexcept -> void *
{
	return ptr;
}

inline auto operator new[](std::size_t, void *ptr) noexcept -> void *
{
	return ptr;
}

inline auto operator delete(void *, void *) noexcept -> void { }
inline auto operator delete[](void *, void *) noexcept -> void { }
