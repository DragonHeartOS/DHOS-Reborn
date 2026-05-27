#pragma once

#include <Katline/Memory/MemoryData.h>

#include <CommonLib/Types.h>

namespace Katline {

namespace Memory {

class MemoryManager {
public:
    static void init(MemoryMap const* mmap);

    static void* allocate(size_t const size);
    static void free(void* ptr);
};

using MM = MemoryManager;

}

}
