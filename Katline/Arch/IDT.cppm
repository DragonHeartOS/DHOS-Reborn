export module Katline:IDT;

import CommonLib;
import :Panic;
import :Sync;

export {
	namespace Katline {

	class IDT {
	public:
		struct [[gnu::packed]] Entry {
			u16 offset0;
			u16 selector;
			u8 ist;
			u8 type_attr;
			u16 offset1;
			u32 offset2;
			u32 ignore;
		};

		static void set_offset(Entry &entry, u64 offset);
		static u64 get_offset(Entry &entry);

		struct [[gnu::packed]] IDTR {
			u16 limit;
			u64 base;
		};

		static void init(u32 lapic_id);
		static void init();
		static void set_handler(u8 vector, void (*handler)());
	};

	}
}

namespace Katline::Debug {
void write_formatted(char const *str, ...);
}

namespace Katline {

[[noreturn]] static auto halt_forever() -> void
{
	for (;;)
		asm("hlt");
}

IDT::IDTR idtr;
IDT::Entry entries[256];

static bool s_idt_built {};
static Sync::SpinLock s_idt_lock;

static_assert(sizeof(IDT::Entry) == 16, "IDT entry must be 16 bytes");
static_assert(sizeof(IDT::IDTR) == 10, "IDTR must be 10 bytes");

auto IDT::set_offset(Entry &entry, u64 offset) -> void
{
	entry.offset0 = (u16)(offset & 0x000000000000ffff);
	entry.offset1 = (u16)((offset & 0x00000000ffff0000) >> 16);
	entry.offset2 = (u32)((offset & 0xffffffff00000000) >> 32);
}

auto IDT::get_offset(Entry &entry) -> u64
{
	u64 offset = 0;
	offset |= (u64)entry.offset0;
	offset |= (u64)entry.offset1 << 16;
	offset |= (u64)entry.offset2 << 32;
	return offset;
}

void IDT::init(u32 lapic_id)
{
	{
		Sync::ScopedIrqSpinLock guard { s_idt_lock };

		if (!s_idt_built) {
			u16 code_selector;
			asm volatile("mov %%cs, %0" : "=r"(code_selector));

			for (u16 i {}; i < 256; i++) {
				entries[i] = {};
				entries[i].selector = code_selector;
				entries[i].ist = 0;
				entries[i].type_attr = 0x8e;
				entries[i].ignore = 0;

				if (i < 32)
					set_offset(entries[i],
					    reinterpret_cast<u64>(
					        Katline::exception_handler_for_vector(
					            static_cast<u8>(i))));
				else
					set_offset(
					    entries[i], reinterpret_cast<u64>(&halt_forever));
			}

			idtr.limit = static_cast<u16>(sizeof(entries) - 1);
			idtr.base = reinterpret_cast<u64>(&entries);

			s_idt_built = true;
		}
	}

	asm volatile("cli");
	asm volatile("lidt %0" : : "m"(idtr));
	Debug::write_formatted("[IDT] Loaded for CPU %d.\n", lapic_id);
}

auto IDT::set_handler(u8 vector, void (*handler)()) -> void
{
	Sync::ScopedIrqSpinLock guard { s_idt_lock };
	set_offset(entries[vector], reinterpret_cast<u64>(handler));
}

}
