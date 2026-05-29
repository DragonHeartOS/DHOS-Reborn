export module Katline:MemoryData;

import CommonLib;

export {
	namespace Katline {

	namespace Memory {

	enum class MemoryType : u32 {
		USABLE = 1,
		RESERVED,
		ACPI_RECLAIMABLE,
		ACPI_NVS,
		BAD_MEMORY,
		BOOTLOADER_RECLAIMABLE = 0x1000,
		KERNEL_AND_MODULES,
		FRAMEBUFFER
	};

	struct MemoryData {
		u64 base;
		u64 size;
		MemoryType type;
		u32 unused;
	};

	struct MemoryMap {
		u64 size;
		MemoryData *data;
	};

	}

	}
}
