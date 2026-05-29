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
	auto *ctx { this };
	asm volatile("movq %%rax, %c[rax](%[ctx])\n\t"
	             "movq %%rbx, %c[rbx](%[ctx])\n\t"
	             "movq %%rcx, %c[rcx](%[ctx])\n\t"
	             "movq %%rdx, %c[rdx](%[ctx])\n\t"
	             "movq %%rsi, %c[rsi](%[ctx])\n\t"
	             "movq %%rdi, %c[rdi](%[ctx])\n\t"
	             "movq %%rbp, %c[rbp](%[ctx])\n\t"
	             "movq %%rsp, %c[rsp](%[ctx])\n\t"
	             "movq %%r8, %c[r8](%[ctx])\n\t"
	             "movq %%r9, %c[r9](%[ctx])\n\t"
	             "movq %%r10, %c[r10](%[ctx])\n\t"
	             "movq %%r11, %c[r11](%[ctx])\n\t"
	             "movq %%r12, %c[r12](%[ctx])\n\t"
	             "movq %%r13, %c[r13](%[ctx])\n\t"
	             "movq %%r14, %c[r14](%[ctx])\n\t"
	             "movq %%r15, %c[r15](%[ctx])\n\t"
	             "pushfq\n\tpopq %%rax\n\t"
	             "movq %%rax, %c[rflags](%[ctx])\n\t"
	    :
	    : [ctx] "r"(ctx), [rax] "i"(__builtin_offsetof(CPUContext, rax)),
	    [rbx] "i"(__builtin_offsetof(CPUContext, rbx)),
	    [rcx] "i"(__builtin_offsetof(CPUContext, rcx)),
	    [rdx] "i"(__builtin_offsetof(CPUContext, rdx)),
	    [rsi] "i"(__builtin_offsetof(CPUContext, rsi)),
	    [rdi] "i"(__builtin_offsetof(CPUContext, rdi)),
	    [rbp] "i"(__builtin_offsetof(CPUContext, rbp)),
	    [rsp] "i"(__builtin_offsetof(CPUContext, rsp)),
	    [r8] "i"(__builtin_offsetof(CPUContext, r8)),
	    [r9] "i"(__builtin_offsetof(CPUContext, r9)),
	    [r10] "i"(__builtin_offsetof(CPUContext, r10)),
	    [r11] "i"(__builtin_offsetof(CPUContext, r11)),
	    [r12] "i"(__builtin_offsetof(CPUContext, r12)),
	    [r13] "i"(__builtin_offsetof(CPUContext, r13)),
	    [r14] "i"(__builtin_offsetof(CPUContext, r14)),
	    [r15] "i"(__builtin_offsetof(CPUContext, r15)),
	    [rflags] "i"(__builtin_offsetof(CPUContext, rflags))
	    : "rax", "memory");
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
	auto const *ctx { this };
	u16 ds { static_cast<u16>(this->ds) };
	u16 es { static_cast<u16>(this->es) };
	u16 fs { static_cast<u16>(this->fs) };
	u16 gs { static_cast<u16>(this->gs) };

	asm volatile("movw %w0, %%ds\n\t"
	             "movw %w1, %%es\n\t"
	             "movw %w2, %%fs\n\t"
	             "movw %w3, %%gs\n\t"
	             "movq %c[rax](%[ctx]), %%rax\n\t"
	             "movq %c[rbx](%[ctx]), %%rbx\n\t"
	             "movq %c[rcx](%[ctx]), %%rcx\n\t"
	             "movq %c[rdx](%[ctx]), %%rdx\n\t"
	             "movq %c[rsi](%[ctx]), %%rsi\n\t"
	             "movq %c[rdi](%[ctx]), %%rdi\n\t"
	             "movq %c[rbp](%[ctx]), %%rbp\n\t"
	             "movq %c[r8](%[ctx]), %%r8\n\t"
	             "movq %c[r9](%[ctx]), %%r9\n\t"
	             "movq %c[r10](%[ctx]), %%r10\n\t"
	             "movq %c[r11](%[ctx]), %%r11\n\t"
	             "movq %c[r12](%[ctx]), %%r12\n\t"
	             "movq %c[r13](%[ctx]), %%r13\n\t"
	             "movq %c[r14](%[ctx]), %%r14\n\t"
	             "movq %c[r15](%[ctx]), %%r15\n\t"
	             "movq %c[rsp](%[ctx]), %%rsp\n\t"
	             "pushq %c[rflags](%[ctx])\n\t"
	             "popfq\n\t"
	             "jmp *%c[rip](%[ctx])\n\t"
	    :
	    : [ds] "r"(ds), [es] "r"(es), [fs] "r"(fs), [gs] "r"(gs),
	    [ctx] "r"(ctx), [rax] "i"(__builtin_offsetof(CPUContext, rax)),
	    [rbx] "i"(__builtin_offsetof(CPUContext, rbx)),
	    [rcx] "i"(__builtin_offsetof(CPUContext, rcx)),
	    [rdx] "i"(__builtin_offsetof(CPUContext, rdx)),
	    [rsi] "i"(__builtin_offsetof(CPUContext, rsi)),
	    [rdi] "i"(__builtin_offsetof(CPUContext, rdi)),
	    [rbp] "i"(__builtin_offsetof(CPUContext, rbp)),
	    [r8] "i"(__builtin_offsetof(CPUContext, r8)),
	    [r9] "i"(__builtin_offsetof(CPUContext, r9)),
	    [r10] "i"(__builtin_offsetof(CPUContext, r10)),
	    [r11] "i"(__builtin_offsetof(CPUContext, r11)),
	    [r12] "i"(__builtin_offsetof(CPUContext, r12)),
	    [r13] "i"(__builtin_offsetof(CPUContext, r13)),
	    [r14] "i"(__builtin_offsetof(CPUContext, r14)),
	    [r15] "i"(__builtin_offsetof(CPUContext, r15)),
	    [rsp] "i"(__builtin_offsetof(CPUContext, rsp)),
	    [rflags] "i"(__builtin_offsetof(CPUContext, rflags)),
	    [rip] "i"(__builtin_offsetof(CPUContext, rip))
	    : "memory");
	__builtin_unreachable();
}

void CPUContext::switch_to(CPUContext *prev, CPUContext *next)
{
	if (!next)
		return;

	if (prev) {
		auto *ctx { prev };
		asm volatile("movq %%rax, %c[rax](%[ctx])\n\t"
		             "movq %%rbx, %c[rbx](%[ctx])\n\t"
		             "movq %%rcx, %c[rcx](%[ctx])\n\t"
		             "movq %%rdx, %c[rdx](%[ctx])\n\t"
		             "movq %%rsi, %c[rsi](%[ctx])\n\t"
		             "movq %%rdi, %c[rdi](%[ctx])\n\t"
		             "movq %%rbp, %c[rbp](%[ctx])\n\t"
		             "movq %%rsp, %c[rsp](%[ctx])\n\t"
		             "movq %%r8, %c[r8](%[ctx])\n\t"
		             "movq %%r9, %c[r9](%[ctx])\n\t"
		             "movq %%r10, %c[r10](%[ctx])\n\t"
		             "movq %%r11, %c[r11](%[ctx])\n\t"
		             "movq %%r12, %c[r12](%[ctx])\n\t"
		             "movq %%r13, %c[r13](%[ctx])\n\t"
		             "movq %%r14, %c[r14](%[ctx])\n\t"
		             "movq %%r15, %c[r15](%[ctx])\n\t"
		             "pushfq\n\tpopq %%rax\n\t"
		             "movq %%rax, %c[rflags](%[ctx])\n\t"
		    :
		    : [ctx] "r"(ctx), [rax] "i"(__builtin_offsetof(CPUContext, rax)),
		    [rbx] "i"(__builtin_offsetof(CPUContext, rbx)),
		    [rcx] "i"(__builtin_offsetof(CPUContext, rcx)),
		    [rdx] "i"(__builtin_offsetof(CPUContext, rdx)),
		    [rsi] "i"(__builtin_offsetof(CPUContext, rsi)),
		    [rdi] "i"(__builtin_offsetof(CPUContext, rdi)),
		    [rbp] "i"(__builtin_offsetof(CPUContext, rbp)),
		    [rsp] "i"(__builtin_offsetof(CPUContext, rsp)),
		    [r8] "i"(__builtin_offsetof(CPUContext, r8)),
		    [r9] "i"(__builtin_offsetof(CPUContext, r9)),
		    [r10] "i"(__builtin_offsetof(CPUContext, r10)),
		    [r11] "i"(__builtin_offsetof(CPUContext, r11)),
		    [r12] "i"(__builtin_offsetof(CPUContext, r12)),
		    [r13] "i"(__builtin_offsetof(CPUContext, r13)),
		    [r14] "i"(__builtin_offsetof(CPUContext, r14)),
		    [r15] "i"(__builtin_offsetof(CPUContext, r15)),
		    [rflags] "i"(__builtin_offsetof(CPUContext, rflags))
		    : "rax", "memory");
		prev->rip = reinterpret_cast<u64>(&&resume);
		prev->cs = current_cs();
		prev->ss = current_ss();
	}

	next->restore_and_enter();

resume:
	asm volatile("" ::: "memory");
}

}
