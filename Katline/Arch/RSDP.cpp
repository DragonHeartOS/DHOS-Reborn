#include <Katline/Arch/RSDP.h>

namespace Katline::Arch {

auto get_sdt_info(RSDP *rsdp) -> SDTInfo
{
	// ACPI 2.0+ has revision >= 2
	if (rsdp->revision >= 2 && rsdp->xsdt_address != 0) {
		return {
			.address = static_cast<uintptr_t>(rsdp->xsdt_address),
			.is_xsdt = true,
		};
	}
	return {
		.address = static_cast<uintptr_t>(rsdp->rsdt_address),
		.is_xsdt = false,
	};
}

}
