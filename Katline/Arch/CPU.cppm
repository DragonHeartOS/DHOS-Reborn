export module Katline:CPU;

import CommonLib;

export {
	namespace Katline::Arch {

	struct CPUIDRegs {
		u32 eax;
		u32 ebx;
		u32 ecx;
		u32 edx;

		static auto cpuid(u32 leaf, u32 subleaf) -> CPUIDRegs;
	};

	struct CPUID {
		u32 max_basic_leaf;
		char vendor_id[13];

		CPUIDRegs leaf1;
		u8 stepping_id;
		u8 model;
		u8 family;
		u8 processor_type;
		u8 ext_model;
		u8 ext_family;
		u8 initial_apic_id;
		bool has_apic;
		bool has_x2apic;

		CPUIDRegs leaf7_subleaf0;
		CPUIDRegs leafB_subleaf0;
		CPUIDRegs leafB_subleaf1;

		static auto query_cpuid() -> CPUID;
	};

	auto rdmsr(u32 msr) -> u64;
	auto wrmsr(u32 msr, u64 value) -> void;
	auto rdtsc() -> u64;

	}
}

namespace Katline::Arch {

auto CPUIDRegs::cpuid(u32 leaf, u32 subleaf) -> CPUIDRegs
{
	CPUIDRegs result {};
	asm volatile("cpuid"
	    : "=a"(result.eax), "=b"(result.ebx), "=c"(result.ecx), "=d"(result.edx)
	    : "a"(leaf), "c"(subleaf));
	return result;
}

auto CPUID::query_cpuid() -> CPUID
{
	CPUID info {};

	auto leaf0 { CPUIDRegs::cpuid(0, 0) };
	info.max_basic_leaf = leaf0.eax;

	for (u8 i {}; i < 4; i++) {
		info.vendor_id[i] = static_cast<char>((leaf0.ebx >> (i * 8)) & 0xff);
		info.vendor_id[4 + i]
		    = static_cast<char>((leaf0.edx >> (i * 8)) & 0xff);
		info.vendor_id[8 + i]
		    = static_cast<char>((leaf0.ecx >> (i * 8)) & 0xff);
	}
	info.vendor_id[12] = '\0';

	auto leaf1 { CPUIDRegs::cpuid(1, 0) };
	info.leaf1 = leaf1;
	info.stepping_id = static_cast<u8>(leaf1.eax & 0x0f);
	info.model = static_cast<u8>((leaf1.eax >> 4) & 0x0f);
	info.family = static_cast<u8>((leaf1.eax >> 8) & 0x0f);
	info.processor_type = static_cast<u8>((leaf1.eax >> 12) & 0x03);
	info.ext_model = static_cast<u8>((leaf1.eax >> 16) & 0x0f);
	info.ext_family = static_cast<u8>((leaf1.eax >> 20) & 0xff);
	info.initial_apic_id = static_cast<u8>((leaf1.ebx >> 24) & 0xff);
	info.has_apic = (leaf1.edx & (1u << 9)) != 0;
	info.has_x2apic = (leaf1.ecx & (1u << 21)) != 0;

	if (info.max_basic_leaf >= 7)
		info.leaf7_subleaf0 = CPUIDRegs::cpuid(7, 0);

	if (info.max_basic_leaf >= 0xB) {
		info.leafB_subleaf0 = CPUIDRegs::cpuid(0xB, 0);
		info.leafB_subleaf1 = CPUIDRegs::cpuid(0xB, 1);
	}

	return info;
}

auto rdmsr(u32 msr) -> u64
{
	u32 low {};
	u32 high {};
	asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
	return (static_cast<u64>(high) << 32) | static_cast<u64>(low);
}

auto wrmsr(u32 msr, u64 value) -> void
{
	u32 low = static_cast<u32>(value & 0xffffffffull);
	u32 high = static_cast<u32>((value >> 32) & 0xffffffffull);
	asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

auto rdtsc() -> u64
{
	u32 low {};
	u32 high {};
	asm volatile("rdtsc" : "=a"(low), "=d"(high));
	return (static_cast<u64>(high) << 32) | static_cast<u64>(low);
}

}
