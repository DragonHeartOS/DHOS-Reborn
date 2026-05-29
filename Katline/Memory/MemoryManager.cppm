export module Katline:MemoryManager;

import CommonLib;
import :MemoryData;

export {
	namespace Katline::Memory {

	class MemoryManager {
	public:
		static void init(MemoryMap const *mmap);

		static void *allocate(usize const size);
		static void *allocate_aligned(usize const size, usize alignment);
		static void free(void *ptr);
	};

	using MM = MemoryManager;

	}
}

namespace Katline {

namespace Debug {
void write_formatted(char const *str, ...);
}

namespace Memory {
void mrvn_memory_init(void *mem, usize size);
bool mrvn_memory_add(void *mem, usize size);
void *mrvn_malloc(usize size);
void mrvn_free(void *mem);
}

}

namespace Katline::Memory {

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

	auto const start { reinterpret_cast<uptr>(raw) + sizeof(AllocationHeader) };
	uptr const aligned { (start + alignment - 1) & ~(alignment - 1) };

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
		    reinterpret_cast<uptr>(ptr) - sizeof(AllocationHeader)),
	};

	if (header->magic == ALIGNED_MAGIC) {
		mrvn_free(header->raw);
		return;
	}

	mrvn_free(ptr);
}

}
