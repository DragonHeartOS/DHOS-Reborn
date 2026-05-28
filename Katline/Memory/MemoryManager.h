#pragma once

#include <Katline/Memory/MemoryData.h>

#include <CommonLib/Types.h>

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
