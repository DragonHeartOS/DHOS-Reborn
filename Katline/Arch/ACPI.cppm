export module Katline:ACPI;

import CommonLib;

export {
	namespace Katline::Arch {

	struct ACPIMADT;

	struct [[gnu::packed]] ACPISDT {
		static auto from(uptr addr) -> ACPISDT *
		{
			// TODO: Validate https://wiki.osdev.org/RSDT#Checksum
			return reinterpret_cast<ACPISDT *>(addr);
		}

		auto find_by_signature(
		    CL::StringView signature, uptr hhdm_offset, bool is_xsdt) -> void *;
		auto find_madt(uptr hhdm_offset, bool is_xsdt) -> ACPIMADT *;

		u8 signature[4];
		u32 length;
		u8 revision;
		u8 checksum;
		u8 oemid[6];
		u8 oem_table_id[8];
		u32 oem_revision;
		u32 creator_id;
		u32 creator_revision;
	};

	namespace APICInterruptControllerStructure {

	struct [[gnu::packed]] Header {
		u8 entry_type;
		u8 record_length;
	};

	struct [[gnu::packed]] ProcessorLocalAPIC {
		enum Flags : u32 {
			ProcessorEnabled = 1 << 0,
			OnlineCapable = 1 << 1,
		};

		Header header;
		u8 acpi_processor_id;
		u8 apic_id;
		Flags flags;
	};

	struct [[gnu::packed]] IOAPIC {
		Header header;
		u8 id;
		u8 reserved;
		u32 apic_address;
		u32 global_system_interrupt_base;
	};

	struct [[gnu::packed]] APICInterruptSourceOverride {
		Header header;
		u8 bus_source;
		u8 irq_source;
		u32 global_system_interrupt;
		u16 flags;
	};

	struct [[gnu::packed]] IOAPICNonMaskableInterruptSource {
		Header header;
		u8 nmi_source;
		u8 reserved;
		u16 flags;
		u32 global_system_interrupt;
	};

	struct [[gnu::packed]] LocalAPICNonMaskableInterrupts {
		Header header;
		u8 acpi_processor_id;
		u16 flags;
		u8 lint;
	};

	struct [[gnu::packed]] LocalAPICAddressOverride {
		Header header;
		u16 reserved;
		u64 physical_address_of_local_apic;
	};

	using V = CL::Variant<ProcessorLocalAPIC *, IOAPIC *,
	    APICInterruptSourceOverride *, IOAPICNonMaskableInterruptSource *,
	    LocalAPICNonMaskableInterrupts *, LocalAPICAddressOverride *, void *>;

	};

	struct [[gnu::packed]] ACPIMADT {
		auto get_interrupt_controller_structures()
		    -> CL::ArrayList<APICInterruptControllerStructure::V>;
		auto get_local_apic_physical_address() -> uptr;

		ACPISDT header;
		u32 local_apic_address;
		u32 flags;
	};

	}
}

namespace Katline::Arch {

auto ACPISDT::find_by_signature(
    CL::StringView signature, uptr hhdm_offset, bool is_xsdt) -> void *
{
	auto cur = reinterpret_cast<uptr>(this) + sizeof(ACPISDT);
	auto end = reinterpret_cast<uptr>(this) + length;
	auto entry_size = is_xsdt ? sizeof(u64) : sizeof(u32);

	while (cur + entry_size <= end) {
		uptr phys = is_xsdt ? static_cast<uptr>(*reinterpret_cast<u64 *>(cur))
		                    : static_cast<uptr>(*reinterpret_cast<u32 *>(cur));

		auto *h = reinterpret_cast<ACPISDT *>(phys + hhdm_offset);

		if (CL::StringView((char const *)h->signature, 4) == signature)
			return h;

		cur += entry_size;
	}

	return nullptr;
}

auto ACPISDT::find_madt(uptr hhdm_offset, bool is_xsdt) -> ACPIMADT *
{
	return reinterpret_cast<ACPIMADT *>(
	    find_by_signature("APIC", hhdm_offset, is_xsdt));
}

auto ACPIMADT::get_interrupt_controller_structures()
    -> CL::ArrayList<APICInterruptControllerStructure::V>
{
	CL::ArrayList<APICInterruptControllerStructure::V> ret;

	auto cur = reinterpret_cast<uptr>(this) + sizeof(ACPIMADT);
	auto end = reinterpret_cast<uptr>(this) + header.length;

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

auto ACPIMADT::get_local_apic_physical_address() -> uptr
{
	uptr ret {};
	auto structs { get_interrupt_controller_structures() };
	structs.iter().for_each([&](APICInterruptControllerStructure::V &v) {
		if (ret)
			return;
		auto val = v.get<
		    APICInterruptControllerStructure::LocalAPICAddressOverride *>();
		if (val)
			ret = val.unwrap()->physical_address_of_local_apic;
	});
	if (!ret)
		ret = local_apic_address;
	return ret;
}

}
