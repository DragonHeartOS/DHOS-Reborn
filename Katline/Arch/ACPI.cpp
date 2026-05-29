#include <Katline/Arch/ACPI.h>

#include <CommonLib/StringView.h>
#include <CommonLib/Types.h>

namespace Katline::Arch {

auto ACPISDT::find_by_signature(
    CL::StringView signature, uintptr_t hhdm_offset, bool is_xsdt) -> void *
{
	auto cur = reinterpret_cast<uintptr_t>(this) + sizeof(ACPISDT);
	auto end = reinterpret_cast<uintptr_t>(this) + length;
	auto entry_size = is_xsdt ? sizeof(u64) : sizeof(u32);

	while (cur + entry_size <= end) {
		uintptr_t phys = is_xsdt
		    ? static_cast<uintptr_t>(*reinterpret_cast<u64 *>(cur))
		    : static_cast<uintptr_t>(*reinterpret_cast<u32 *>(cur));

		auto *h = reinterpret_cast<ACPISDT *>(phys + hhdm_offset);

		if (CL::StringView((char const *)h->signature, 4) == signature)
			return h;

		cur += entry_size;
	}

	return nullptr;
}

auto ACPISDT::find_madt(uintptr_t hhdm_offset, bool is_xsdt) -> ACPIMADT *
{
	return reinterpret_cast<ACPIMADT *>(
	    find_by_signature("APIC", hhdm_offset, is_xsdt));
}

auto ACPIMADT::get_interrupt_controller_structures()
    -> CL::ArrayList<APICInterruptControllerStructure::V>
{
	CL::ArrayList<APICInterruptControllerStructure::V> ret;

	auto cur = reinterpret_cast<uintptr_t>(this) + sizeof(ACPIMADT);
	auto end = reinterpret_cast<uintptr_t>(this) + header.length;

	while (cur + sizeof(APICInterruptControllerStructure::Header) <= end) {
		auto *hdr
		    = reinterpret_cast<APICInterruptControllerStructure::Header *>(cur);

		if (hdr->record_length
		    < sizeof(APICInterruptControllerStructure::Header))
			break;

		if (cur + hdr->record_length > end)
			break;

		APICInterruptControllerStructure::V v { (void *)nullptr };

		switch (hdr->entry_type) {
		case 0:
			v = reinterpret_cast<
			    APICInterruptControllerStructure::ProcessorLocalAPIC *>(hdr);
			break;
		case 1:
			v = reinterpret_cast<APICInterruptControllerStructure::IOAPIC *>(
			    hdr);
			break;
		case 2:
			v = reinterpret_cast<APICInterruptControllerStructure::
			        APICInterruptSourceOverride *>(hdr);
			break;
		case 3:
			v = reinterpret_cast<APICInterruptControllerStructure::
			        IOAPICNonMaskableInterruptSource *>(hdr);
			break;
		case 4:
			v = reinterpret_cast<APICInterruptControllerStructure::
			        LocalAPICNonMaskableInterrupts *>(hdr);
			break;
		case 5:
			v = reinterpret_cast<
			    APICInterruptControllerStructure::LocalAPICAddressOverride *>(
			    hdr);
			break;
		default:
			cur += hdr->record_length;
			continue;
		}

		ret.push(v);
		cur += hdr->record_length;
	}

	return ret;
}

auto ACPIMADT::get_local_apic_physical_address() -> uintptr_t
{
	uintptr_t ret {};
	auto structs { get_interrupt_controller_structures() };
	structs.iter().for_each([&](APICInterruptControllerStructure::V &v) {
		if (ret)
			return;
		auto val = v.get<
		    APICInterruptControllerStructure::LocalAPICAddressOverride *>();
		if (val) {
			ret = val.unwrap()->physical_address_of_local_apic;
		}
	});
	if (!ret) {
		ret = local_apic_address;
	}
	return ret;
}

}
