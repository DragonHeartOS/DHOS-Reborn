#pragma once

#include <CommonLib/Types.h>

namespace Katline::Arch {

struct [[gnu::packed]] RSDP {
	char signature[8];
	u8 checksum;
	char oemid[6];
	u8 revision;
	u32 rsdt_address;

	// rev 2
	u32 length;
	u64 xsdt_address;
	u8 extended_checksum;
	u8 reserved[3];
};

struct SDTInfo {
	uintptr_t address;
	bool is_xsdt;
};

auto get_sdt_address(RSDP *) -> SDTInfo;

}
