export module Katline:RSDP;

import CommonLib;

export {
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
		uptr address;
		bool is_xsdt;
	};

	auto get_sdt_info(RSDP *) -> SDTInfo;

	}
}

namespace Katline::Arch {

auto get_sdt_info(RSDP *rsdp) -> SDTInfo
{
	if (rsdp->revision >= 2 && rsdp->xsdt_address != 0) {
		return {
			.address = static_cast<uptr>(rsdp->xsdt_address),
			.is_xsdt = true,
		};
	}

	return {
		.address = static_cast<uptr>(rsdp->rsdt_address),
		.is_xsdt = false,
	};
}

}
