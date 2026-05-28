#include <Katline/Memory/MemoryManager.h>

#include <CommonLib/Types.h>
#include <Katline/Debug.h>
#include <Katline/Memory/Heap.h>
#include <Katline/Memory/MemoryData.h>

namespace Katline {

namespace Memory {

void MemoryManager::init(MemoryMap const *mmap)
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

void *MemoryManager::allocate(size_t const size) { return mrvn_malloc(size); }

struct AllocationHeader {
	void *raw;
	u64 magic;
};

constexpr u64 ALIGNED_MAGIC = 0xA7B04F1EULL;

void *MemoryManager::allocate_aligned(size_t const size, size_t alignment)
{
	if (alignment < sizeof(void *))
		alignment = sizeof(void *);

	if ((alignment & (alignment - 1)) != 0)
		return nullptr;

	size_t const total { size + alignment - 1 + sizeof(AllocationHeader) };

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

void MemoryManager::free(void *ptr)
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
