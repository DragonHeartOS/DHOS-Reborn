#pragma once

#include <CommonLib/Types.h>

namespace Katline {

class IDT {
public:
	struct [[gnu::packed]] Entry {
		u16 offset0;
		u16 selector;
		u8 ist;
		u8 type_attr;
		u16 offset1;
		u32 offset2;
		u32 ignore;
	};

	static void set_offset(Entry &entry, u64 offset);
	static u64 get_offset(Entry &entry);

	struct [[gnu::packed]] IDTR {
		u16 limit;
		u64 base;
	};

	static void init();
};

}
