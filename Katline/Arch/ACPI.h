#pragma once

#include <CommonLib/ArrayList.h>
#include <CommonLib/StringView.h>
#include <CommonLib/Types.h>
#include <CommonLib/Variant.h>

namespace Katline::Arch {

struct ACPIMADT;

struct [[gnu::packed]] ACPISDT {
	static auto from(uintptr_t addr) -> ACPISDT *
	{
		// TODO: Validate https://wiki.osdev.org/RSDT#Checksum
		return reinterpret_cast<ACPISDT *>(addr);
	}

	auto find_by_signature(CL::StringView signature, uintptr_t hhdm_offset,
	    bool is_xsdt) -> void *;
	auto find_madt(uintptr_t hhdm_offset, bool is_xsdt) -> ACPIMADT *;

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
	auto get_local_apic_physical_address() -> uintptr_t;

	ACPISDT header;
	u32 local_apic_address;
	u32 flags;
};

}
