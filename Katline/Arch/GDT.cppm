export module Katline:GDT;

import CommonLib;
import :CPU;
import :Debug;

export {
	namespace Katline::Arch {
	constexpr u16 user_code_selector { 0x1b };
	constexpr u16 user_data_selector { 0x23 };

	struct [[gnu::packed]] GDTEntry {
		u16 limit0 {};
		u16 base0 {};
		u8 base1 {};
		u8 access_byte {};
		u8 limit1 : 4 {};
		u8 flags : 4 {};
		u8 base2 {};
	};

	struct [[gnu::packed]] TSSEntry {
		u16 limit0;
		u16 base0;
		u8 base1;
		u8 access_byte;
		u8 limit1 : 4;
		u8 flags : 4;
		u8 base2;
		u32 base3;
		u32 reserved;
	};

	struct [[gnu::packed]] TSS {
		u32 reserved0;
		u64 rsp0;
		u64 rsp1;
		u64 rsp2;
		u64 reserved1;
		u64 ist1;
		u64 ist2;
		u64 ist3;
		u64 ist4;
		u64 ist5;
		u64 ist6;
		u64 ist7;
		u64 reserved2;
		u16 reserved3;
		u16 iomap_base;
	};

	struct [[gnu::packed]] GDT {
		GDTEntry null;
		GDTEntry kernel_code;
		GDTEntry kernel_data;
		GDTEntry user_code;
		GDTEntry user_data;
		TSSEntry tss;

		void load(u32 lapic_id, TSS &tss);
	};

	struct [[gnu::packed]] GDTR {
		u16 limit;
		u64 base;
	};

	extern "C" auto katline_current_kernel_stack_top() -> u64;

	}
}

namespace Katline::Arch {

export auto set_kernel_stack_for_current_cpu(uptr stack_top) -> void;

constexpr GDT gdt_template {
	.null = {},

	.kernel_code = {
		.limit0 = 0xFFFF,
		.base0 = 0,
		.base1 = 0,
		.access_byte = 0x9A,
		.limit1 = 0xF,
		.flags = 0xA,
		.base2 = 0,
	},

	.kernel_data = {
		.limit0 = 0xFFFF,
		.base0 = 0,
		.base1 = 0,
		.access_byte = 0x92,
		.limit1 = 0xF,
		.flags = 0xC,
		.base2 = 0,
	},

	.user_code = {
		.limit0 = 0xFFFF,
		.base0 = 0,
		.base1 = 0,
		.access_byte = 0xFA,
		.limit1 = 0xF,
		.flags = 0xA,
		.base2 = 0,
	},

	.user_data = {
		.limit0 = 0xFFFF,
		.base0 = 0,
		.base1 = 0,
		.access_byte = 0xF2,
		.limit1 = 0xF,
		.flags = 0xC,
		.base2 = 0,
	},

	.tss = {},
};

static constexpr usize max_tss_slots { 256 };
static TSS *s_tss_by_lapic_id[max_tss_slots] {};

static void init_tss_descriptor(GDT &gdt, TSS &tss)
{
	tss = {};
	tss.iomap_base = sizeof(TSS);

	u64 base = reinterpret_cast<u64>(&tss);
	u32 limit = sizeof(TSS) - 1;

	gdt.tss.limit0 = limit & 0xFFFF;
	gdt.tss.base0 = base & 0xFFFF;
	gdt.tss.base1 = (base >> 16) & 0xFF;
	gdt.tss.access_byte = 0x89; // present, ring 0, available 64-bit TSS
	gdt.tss.limit1 = (limit >> 16) & 0xF;
	gdt.tss.flags = 0;
	gdt.tss.base2 = (base >> 24) & 0xFF;
	gdt.tss.base3 = (base >> 32) & 0xFFFFFFFF;
	gdt.tss.reserved = 0;
}

void GDT::load(u32 lapic_id, TSS &tss)
{
	*this = gdt_template;
	init_tss_descriptor(*this, tss);
	if (lapic_id < max_tss_slots)
		s_tss_by_lapic_id[lapic_id] = &tss;

	GDTR gdtr {
		.limit = sizeof(*this) - 1,
		.base = reinterpret_cast<u64>(this),
	};

	asm volatile("cli");
	asm volatile("lgdt %0" : : "m"(gdtr) : "memory");

	// Reload segment registers
	asm volatile("mov $0x10, %%ax\n"
	             "mov %%ax, %%ds\n"
	             "mov %%ax, %%es\n"
	             "mov %%ax, %%ss\n"

	             "xor %%ax, %%ax\n"
	             "mov %%ax, %%fs\n"
	             "mov %%ax, %%gs\n"

	             "pushq $0x08\n"
	             "leaq 1f(%%rip), %%rax\n"
	             "pushq %%rax\n"
	             "lretq\n"
	             "1:\n"

	             "mov $0x28, %%ax\n"
	             "ltr %%ax\n"
	    :
	    :
	    : "rax", "memory");
	asm volatile("sti");

	Debug::print_formatted("[GDT] Loaded GDT for CPU %d.\n", lapic_id);
}

auto set_kernel_stack_for_current_cpu(uptr stack_top) -> void
{
	auto const lapic_id { current_lapic_id() };
	if (lapic_id >= max_tss_slots || !s_tss_by_lapic_id[lapic_id])
		return;
	s_tss_by_lapic_id[lapic_id]->rsp0 = stack_top;
}

extern "C" auto katline_current_kernel_stack_top() -> u64
{
	auto const lapic_id { current_lapic_id() };
	if (lapic_id >= max_tss_slots || !s_tss_by_lapic_id[lapic_id])
		return 0;
	return s_tss_by_lapic_id[lapic_id]->rsp0;
}

}
