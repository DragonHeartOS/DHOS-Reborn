#include "Debug.h"
#include <Katline/Arch/IDT.h>

#include <Katline/Arch/Interrupts.h>

namespace Katline {

struct interrupt_frame {
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
};

[[noreturn]] static void halt_forever()
{
    for (;;)
        asm("hlt");
}

extern "C" __attribute__((interrupt)) void idt_default_handler(interrupt_frame*)
{
    halt_forever();
}

extern "C" __attribute__((interrupt)) void idt_default_handler_ec(interrupt_frame*, u64)
{
    halt_forever();
}

static bool vector_has_error_code(u8 vector)
{
    switch (vector) {
    case 8:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 17:
    case 21:
    case 29:
    case 30:
        return true;
    default:
        return false;
    }
}

IDT::IDTR idtr;
IDT::Entry entries[256];

static_assert(sizeof(IDT::Entry) == 16, "IDT entry must be 16 bytes");
static_assert(sizeof(IDT::IDTR) == 10, "IDTR must be 10 bytes");

void IDT::SetOffset(Entry& entry, u64 offset)
{
    entry.offset0 = (u16)(offset & 0x000000000000ffff);
    entry.offset1 = (u16)((offset & 0x00000000ffff0000) >> 16);
    entry.offset2 = (u32)((offset & 0xffffffff00000000) >> 32);
}

u64 IDT::GetOffset(Entry& entry)
{
    u64 offset = 0;
    offset |= (u64)entry.offset0;
    offset |= (u64)entry.offset1 << 16;
    offset |= (u64)entry.offset2 << 32;
    return offset;
}

void IDT::Init()
{
    u16 code_selector;
    asm volatile("mov %%cs, %0"
                 : "=r"(code_selector));

    for (u16 i = 0; i < 256; i++) {
        entries[i] = {};
        entries[i].selector = code_selector;
        entries[i].ist = 0;
        entries[i].type_attr = 0x8e;
        entries[i].ignore = 0;

        if (vector_has_error_code((u8)i))
            SetOffset(entries[i], (u64)&idt_default_handler_ec);
        else
            SetOffset(entries[i], (u64)&idt_default_handler);
    }

    idtr.limit = (u16)(sizeof(entries) - 1);
    idtr.base = (u64)&entries;

    asm volatile("cli");
    asm volatile("lidt %0"
        :
        : "m"(idtr));

    Debug::WriteFormatted("[IDT]: Loaded.\n");
}

}
