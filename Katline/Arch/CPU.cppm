export module Katline:CPU;

import CommonLib;

export {
	namespace Katline::Arch {

	struct CPUContext {
		u64 rax {}, rbx {}, rcx {}, rdx {};
		u64 rsi {}, rdi {};
		u64 rbp {}, rsp {};
		u64 r8 {}, r9 {}, r10 {}, r11 {}, r12 {}, r13 {}, r14 {}, r15 {};

		u64 rip {};
		u64 rflags {};

		u64 cs {};
		u64 ss {};
		u64 ds {};
		u64 es {};
		u64 fs {};
		u64 gs {};

		[[gnu::noinline]] void save_current();
		[[gnu::noinline]] [[noreturn]] void restore_and_enter() const;
		[[gnu::noinline]] static void switch_to(
		    CPUContext *prev, CPUContext *next);
	};

	struct CPUIDRegs {
		u32 eax;
		u32 ebx;
		u32 ecx;
		u32 edx;

		static auto cpuid(u32 leaf, u32 subleaf) -> CPUIDRegs;
		static auto serialize() -> void;
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
	auto current_rsp() -> uptr;
	auto current_cr3() -> uptr;
	auto current_cs() -> uptr;
	auto current_ss() -> uptr;
	auto current_lapic_id() -> u32;
	auto load_cr3(uptr phys) -> void;
	auto invlpg(void *addr) -> void;
	auto rdtsc() -> u64;
	auto rdtscp() -> u64;

	}
}

namespace Katline::Arch {

auto CPUIDRegs::cpuid(u32 leaf, u32 subleaf) -> CPUIDRegs
{
	auto result { CPUIDRegs {} };
	asm volatile("cpuid"
	    : "=a"(result.eax), "=b"(result.ebx), "=c"(result.ecx), "=d"(result.edx)
	    : "a"(leaf), "c"(subleaf));
	return result;
}

auto CPUIDRegs::serialize() -> void
{
	asm volatile("cpuid" : : "a"(0), "c"(0) : "rbx", "rdx");
}

auto CPUID::query_cpuid() -> CPUID
{
	auto info { CPUID {} };

	auto const leaf0 { CPUIDRegs::cpuid(0, 0) };
	info.max_basic_leaf = leaf0.eax;

	for (u8 i {}; i < 4; ++i) {
		info.vendor_id[i] = static_cast<char>((leaf0.ebx >> (i * 8)) & 0xff);
		info.vendor_id[4 + i]
		    = static_cast<char>((leaf0.edx >> (i * 8)) & 0xff);
		info.vendor_id[8 + i]
		    = static_cast<char>((leaf0.ecx >> (i * 8)) & 0xff);
	}
	info.vendor_id[12] = '\0';

	auto const leaf1 { CPUIDRegs::cpuid(1, 0) };
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
	u32 const low { static_cast<u32>(value & 0xffffffffull) };
	u32 const high { static_cast<u32>((value >> 32) & 0xffffffffull) };
	asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

auto current_rsp() -> uptr
{
	auto value { uptr {} };
	asm volatile("mov %%rsp, %0" : "=r"(value));
	return value;
}

auto current_cr3() -> uptr
{
	uptr value {};
	asm volatile("mov %%cr3, %0" : "=r"(value));
	return value;
}

auto current_cs() -> uptr
{
	u16 value {};
	asm volatile("mov %%cs, %0" : "=r"(value));
	return value;
}

auto current_ss() -> uptr
{
	u16 value {};
	asm volatile("mov %%ss, %0" : "=r"(value));
	return value;
}

auto current_lapic_id() -> u32
{
	return static_cast<u32>(rdmsr(0x802) & 0xffffffffull);
}

auto load_cr3(uptr phys) -> void
{
	asm volatile("mov %0, %%cr3" : : "r"(phys) : "memory");
}

auto invlpg(void *addr) -> void
{
	asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

auto rdtsc() -> u64
{
	u32 low {};
	u32 high {};
	asm volatile("rdtsc" : "=a"(low), "=d"(high));
	return (static_cast<u64>(high) << 32) | static_cast<u64>(low);
}

auto rdtscp() -> u64
{
	u32 low {};
	u32 high {};
	u32 aux {};
	asm volatile("rdtscp" : "=a"(low), "=d"(high), "=c"(aux));
	CL::ignore_unused(aux);
	return (static_cast<u64>(high) << 32) | static_cast<u64>(low);
}

void CPUContext::save_current()
{
	asm volatile("mov %%rbx, %0" : "=m"(rbx) : : "memory");
	asm volatile("mov %%rbp, %0" : "=m"(rbp) : : "memory");
	asm volatile("mov %%rsp, %0" : "=m"(rsp) : : "memory");
	asm volatile("mov %%r12, %0" : "=m"(r12) : : "memory");
	asm volatile("mov %%r13, %0" : "=m"(r13) : : "memory");
	asm volatile("mov %%r14, %0" : "=m"(r14) : : "memory");
	asm volatile("mov %%r15, %0" : "=m"(r15) : : "memory");
	asm volatile("pushfq\n\tpopq %0" : "=m"(rflags) : : "memory");
	rip = reinterpret_cast<u64>(__builtin_return_address(0));
	cs = current_cs();
	ss = current_ss();
	ds = current_ss();
	es = current_ss();
	fs = current_ss();
	gs = current_ss();
}

[[noreturn]] void CPUContext::restore_and_enter() const
{
	u16 ds16 { static_cast<u16>(ds) };
	u16 es16 { static_cast<u16>(es) };
	u16 fs16 { static_cast<u16>(fs) };
	u16 gs16 { static_cast<u16>(gs) };

	asm volatile("movw %0, %%ds" : : "rm"(ds16) : "memory");
	asm volatile("movw %0, %%es" : : "rm"(es16) : "memory");
	asm volatile("movw %0, %%fs" : : "rm"(fs16) : "memory");
	asm volatile("movw %0, %%gs" : : "rm"(gs16) : "memory");

	asm volatile("mov %0, %%rbx" : : "m"(rbx) : "rbx", "memory");
	asm volatile("mov %0, %%rbp" : : "m"(rbp) : "rbp", "memory");
	asm volatile("mov %0, %%r12" : : "m"(r12) : "r12", "memory");
	asm volatile("mov %0, %%r13" : : "m"(r13) : "r13", "memory");
	asm volatile("mov %0, %%r14" : : "m"(r14) : "r14", "memory");
	asm volatile("mov %0, %%r15" : : "m"(r15) : "r15", "memory");

	asm volatile("mov %0, %%rsp" : : "m"(rsp) : "rsp", "memory");
	asm volatile("pushq %0\n\tpopfq" : : "m"(rflags) : "cc", "memory");
	asm volatile("jmp *%0" : : "m"(rip) : "memory");
	__builtin_unreachable();
}

[[gnu::naked]] void CPUContext::switch_to(
    CPUContext * /* prev */, CPUContext * /* next */)
{
	asm volatile("testq %rsi, %rsi\n\t"
	             "jz 2f\n\t"
	             "testq %rdi, %rdi\n\t"
	             "jz 0f\n\t"

	             "movq %rbx, 0x08(%rdi)\n\t"
	             "movq %rbp, 0x30(%rdi)\n\t"
	             "movq %rsp, 0x38(%rdi)\n\t"
	             "movq %r12, 0x60(%rdi)\n\t"
	             "movq %r13, 0x68(%rdi)\n\t"
	             "movq %r14, 0x70(%rdi)\n\t"
	             "movq %r15, 0x78(%rdi)\n\t"
	             "leaq 1f(%rip), %rax\n\t"
	             "movq %rax, 0x80(%rdi)\n\t"
	             "pushfq\n\t"
	             "popq 0x88(%rdi)\n\t"
	             "mov %cs, %ax\n\t"
	             "movzx %ax, %rax\n\t"
	             "movq %rax, 0x90(%rdi)\n\t"
	             "mov %ss, %ax\n\t"
	             "movzx %ax, %rax\n\t"
	             "movq %rax, 0x98(%rdi)\n\t"

	             "0:\n\t"
	             "movw 0xa0(%rsi), %ax\n\t"
	             "movw %ax, %ds\n\t"
	             "movw 0xa8(%rsi), %ax\n\t"
	             "movw %ax, %es\n\t"
	             "movw 0xb0(%rsi), %ax\n\t"
	             "movw %ax, %fs\n\t"
	             "movw 0xb8(%rsi), %ax\n\t"
	             "movw %ax, %gs\n\t"

	             "movq 0x08(%rsi), %rbx\n\t"
	             "movq 0x30(%rsi), %rbp\n\t"
	             "movq 0x60(%rsi), %r12\n\t"
	             "movq 0x68(%rsi), %r13\n\t"
	             "movq 0x70(%rsi), %r14\n\t"
	             "movq 0x78(%rsi), %r15\n\t"
	             "movq 0x90(%rsi), %rax\n\t"
	             "mov %cs, %dx\n\t"
	             "movzx %dx, %rdx\n\t"
	             "cmpq %rdx, %rax\n\t"
	             "jne 3f\n\t"
	             "movq 0x38(%rsi), %rsp\n\t"
	             "pushq 0x88(%rsi)\n\t"
	             "popfq\n\t"
	             "jmp *0x80(%rsi)\n\t"

	             "3:\n\t"
	             "pushq 0x98(%rsi)\n\t"
	             "pushq 0x38(%rsi)\n\t"
	             "pushq 0x88(%rsi)\n\t"
	             "pushq 0x90(%rsi)\n\t"
	             "pushq 0x80(%rsi)\n\t"
	             "iretq\n\t"

	             "1:\n\t"
	             "ret\n\t"
	             "2:\n\t"
	             "ret\n\t");
}

}
