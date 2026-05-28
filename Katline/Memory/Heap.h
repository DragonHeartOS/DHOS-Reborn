#pragma once

#include <CommonLib/Types.h>

namespace Katline {

namespace Memory {

void mrvn_memory_init(void *mem, usize size);
bool mrvn_memory_add(void *mem, usize size);
void *mrvn_malloc(usize size);
void mrvn_free(void *mem);

}

}
