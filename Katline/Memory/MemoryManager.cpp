#include <Katline/Memory/MemoryManager.h>

#include <CommonLib/Types.h>
#include <Katline/Debug.h>
#include <Katline/Memory/Heap.h>
#include <Katline/Memory/MemoryData.h>

namespace Katline {

namespace Memory {

void MemoryManager::init(MemoryMap const* mmap)
{
    bool initialized = false;

    for (u64 i = 0; i < mmap->size; i++) {
        MemoryData* md = &mmap->data[i];

        if (md->type != MemoryType::USABLE || md->size == 0)
            continue;

        if (!initialized) {
            mrvn_memory_init((void*)md->base, md->size);
            initialized = true;
            continue;
        }

        mrvn_memory_add((void*)md->base, md->size);
    }

    if (!initialized) {
        Debug::write_formatted("[MemoryManager] Failed to initialize: no usable memory region.\n");
        for (;;)
            asm("hlt");
    }

    Debug::write_formatted("[MemoryManager] Initialized.\n");
}

void* MemoryManager::allocate(size_t size)
{
    return mrvn_malloc(size);
}

void MemoryManager::free(void* ptr)
{
    mrvn_free(ptr);
}

}

}
