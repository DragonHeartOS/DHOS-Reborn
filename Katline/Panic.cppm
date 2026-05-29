export module Katline:Panic;

import CommonLib;
import :Debug;

export {
	namespace Katline {

	struct ExceptionRegisters {
		u64 rax;
		u64 rbx;
		u64 rcx;
		u64 rdx;
		u64 rsi;
		u64 rdi;
		u64 rbp;
		u64 r8;
		u64 r9;
		u64 r10;
		u64 r11;
		u64 r12;
		u64 r13;
		u64 r14;
		u64 r15;

		u64 rip;
		u64 cs;
		u64 rflags;
		u64 rsp;
		u64 ss;

		u8 vector;
		CL::Option<u64> error_code;
	};

	[[noreturn]] auto kpanic(CL::StringView message,
	    CL::Option<ExceptionRegisters const &> regs) -> void;
	[[noreturn]] auto kpanic(CL::StringView message) -> void;

	auto exception_handler_for_vector(u8 vector) -> void (*)();

	}
}

namespace Katline {

struct SavedRegisters {
	u64 rax;
	u64 rbx;
	u64 rcx;
	u64 rdx;
	u64 rsi;
	u64 rdi;
	u64 rbp;
	u64 r8;
	u64 r9;
	u64 r10;
	u64 r11;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;
};

struct ExceptionFrame {
	u64 rip;
	u64 cs;
	u64 rflags;
	u64 rsp;
	u64 ss;
};

static auto read_ss() -> u64
{
	u16 ss {};
	asm volatile("mov %%ss, %0" : "=r"(ss));
	return static_cast<u64>(ss);
}

static auto print_exception_registers(ExceptionRegisters const &regs) -> void
{
	Debug::write_formatted("vector=0x%02x\n", static_cast<u64>(regs.vector));
	if (regs.error_code)
		Debug::write_formatted("error_code=0x%016x\n", *regs.error_code);

	Debug::write_formatted("rax=0x%016x rbx=0x%016x rcx=0x%016x rdx=0x%016x\n",
	    regs.rax, regs.rbx, regs.rcx, regs.rdx);
	Debug::write_formatted("rsi=0x%016x rdi=0x%016x rbp=0x%016x rsp=0x%016x\n",
	    regs.rsi, regs.rdi, regs.rbp, regs.rsp);
	Debug::write_formatted("r8 =0x%016x r9 =0x%016x r10=0x%016x r11=0x%016x\n",
	    regs.r8, regs.r9, regs.r10, regs.r11);
	Debug::write_formatted("r12=0x%016x r13=0x%016x r14=0x%016x r15=0x%016x\n",
	    regs.r12, regs.r13, regs.r14, regs.r15);
	Debug::write_formatted("rip=0x%016x cs=0x%016x rflags=0x%016x ss=0x%016x\n",
	    regs.rip, regs.cs, regs.rflags, regs.ss);
}

[[noreturn]] static auto panic_loop() -> void
{
	for (;;)
		asm volatile("hlt");
}

extern "C" [[noreturn]] auto katline_exception_dispatch(u8 vector,
    SavedRegisters const *saved, ExceptionFrame const *frame,
    bool has_error_code) -> void
{
	ExceptionRegisters regs {};
	regs.rax = saved->rax;
	regs.rbx = saved->rbx;
	regs.rcx = saved->rcx;
	regs.rdx = saved->rdx;
	regs.rsi = saved->rsi;
	regs.rdi = saved->rdi;
	regs.rbp = saved->rbp;
	regs.r8 = saved->r8;
	regs.r9 = saved->r9;
	regs.r10 = saved->r10;
	regs.r11 = saved->r11;
	regs.r12 = saved->r12;
	regs.r13 = saved->r13;
	regs.r14 = saved->r14;
	regs.r15 = saved->r15;
	regs.rip = frame->rip;
	regs.cs = frame->cs;
	regs.rflags = frame->rflags;
	regs.vector = vector;

	if ((frame->cs & 0x3u) != 0) {
		regs.rsp = frame->rsp;
		regs.ss = frame->ss;
	} else {
		regs.rsp = reinterpret_cast<u64>(saved) + sizeof(SavedRegisters);
		regs.ss = read_ss();
	}

	if (has_error_code) {
		auto const *error_code_ptr = reinterpret_cast<u64 const *>(frame) - 1;
		regs.error_code = *error_code_ptr;
	}

	kpanic("Unhandled CPU exception",
	    CL::Option<ExceptionRegisters const &>(regs));
}

#define KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(VEC) \
	extern "C" [[gnu::naked]] auto katline_exception_handler_##VEC() -> void \
	{ \
		asm volatile("cld\n" \
		             "push %r15\n" \
		             "push %r14\n" \
		             "push %r13\n" \
		             "push %r12\n" \
		             "push %r11\n" \
		             "push %r10\n" \
		             "push %r9\n" \
		             "push %r8\n" \
		             "push %rbp\n" \
		             "push %rdi\n" \
		             "push %rsi\n" \
		             "push %rdx\n" \
		             "push %rcx\n" \
		             "push %rbx\n" \
		             "push %rax\n" \
		             "mov $" #VEC ", %edi\n" \
		             "mov %rsp, %rsi\n" \
		             "lea 0x78(%rsp), %rdx\n" \
		             "xor %ecx, %ecx\n" \
		             "call katline_exception_dispatch\n" \
		             "ud2\n"); \
	}

#define KATLINE_DEFINE_EXCEPTION_HANDLER_EC(VEC) \
	extern "C" [[gnu::naked]] auto katline_exception_handler_##VEC() -> void \
	{ \
		asm volatile("cld\n" \
		             "push %r15\n" \
		             "push %r14\n" \
		             "push %r13\n" \
		             "push %r12\n" \
		             "push %r11\n" \
		             "push %r10\n" \
		             "push %r9\n" \
		             "push %r8\n" \
		             "push %rbp\n" \
		             "push %rdi\n" \
		             "push %rsi\n" \
		             "push %rdx\n" \
		             "push %rcx\n" \
		             "push %rbx\n" \
		             "push %rax\n" \
		             "mov $" #VEC ", %edi\n" \
		             "mov %rsp, %rsi\n" \
		             "lea 0x80(%rsp), %rdx\n" \
		             "mov $1, %ecx\n" \
		             "call katline_exception_dispatch\n" \
		             "ud2\n"); \
	}

KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(0)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(1)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(2)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(3)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(4)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(5)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(6)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(7)
KATLINE_DEFINE_EXCEPTION_HANDLER_EC(8)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(9)
KATLINE_DEFINE_EXCEPTION_HANDLER_EC(10)
KATLINE_DEFINE_EXCEPTION_HANDLER_EC(11)
KATLINE_DEFINE_EXCEPTION_HANDLER_EC(12)
KATLINE_DEFINE_EXCEPTION_HANDLER_EC(13)
KATLINE_DEFINE_EXCEPTION_HANDLER_EC(14)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(15)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(16)
KATLINE_DEFINE_EXCEPTION_HANDLER_EC(17)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(18)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(19)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(20)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(21)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(22)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(23)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(24)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(25)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(26)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(27)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(28)
KATLINE_DEFINE_EXCEPTION_HANDLER_EC(29)
KATLINE_DEFINE_EXCEPTION_HANDLER_EC(30)
KATLINE_DEFINE_EXCEPTION_HANDLER_NOEC(31)

static void (*const exception_handlers[32])() = {
	&katline_exception_handler_0,
	&katline_exception_handler_1,
	&katline_exception_handler_2,
	&katline_exception_handler_3,
	&katline_exception_handler_4,
	&katline_exception_handler_5,
	&katline_exception_handler_6,
	&katline_exception_handler_7,
	&katline_exception_handler_8,
	&katline_exception_handler_9,
	&katline_exception_handler_10,
	&katline_exception_handler_11,
	&katline_exception_handler_12,
	&katline_exception_handler_13,
	&katline_exception_handler_14,
	&katline_exception_handler_15,
	&katline_exception_handler_16,
	&katline_exception_handler_17,
	&katline_exception_handler_18,
	&katline_exception_handler_19,
	&katline_exception_handler_20,
	&katline_exception_handler_21,
	&katline_exception_handler_22,
	&katline_exception_handler_23,
	&katline_exception_handler_24,
	&katline_exception_handler_25,
	&katline_exception_handler_26,
	&katline_exception_handler_27,
	&katline_exception_handler_28,
	&katline_exception_handler_29,
	&katline_exception_handler_30,
	&katline_exception_handler_31,
};

auto exception_handler_for_vector(u8 vector) -> void (*)()
{
	return exception_handlers[vector];
}

auto kpanic(CL::StringView message, CL::Option<ExceptionRegisters const &> regs)
    -> void
{
	asm volatile("cli");
	Debug::write_formatted("-=- KERNEL PANIC -=-\n");
	Debug::write_formatted("%.*s\n\n", message.size(), message.data());
	if (regs)
		print_exception_registers(*regs);
	Debug::write_formatted(
	    "\nAll wings are grounded. The heart is silent.\nSystem halted.\n");
	panic_loop();
}

auto kpanic(CL::StringView message) -> void
{
	kpanic(message, CL::Option<ExceptionRegisters const &> {});
}

}
