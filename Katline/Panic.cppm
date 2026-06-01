export module Katline:Panic;

import CommonLib;
import :Debug;
import :KernelSymbols;

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
		CL::Option<u64> cr2;
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

static auto exception_name(u8 vector) -> char const *
{
	switch (vector) {
	case 0:
		return "#DE";
	case 1:
		return "#DB";
	case 2:
		return "#NMI";
	case 3:
		return "#BP";
	case 4:
		return "#OF";
	case 5:
		return "#BR";
	case 6:
		return "#UD";
	case 7:
		return "#NM";
	case 8:
		return "#DF";
	case 10:
		return "#TS";
	case 11:
		return "#NP";
	case 12:
		return "#SS";
	case 13:
		return "#GP";
	case 14:
		return "#PF";
	case 16:
		return "#MF";
	case 17:
		return "#AC";
	case 18:
		return "#MC";
	case 19:
		return "#XM";
	default:
		return "unknown";
	}
}

static auto exception_description(u8 vector) -> char const *
{
	switch (vector) {
	case 0:
		return "divide error";
	case 1:
		return "debug";
	case 2:
		return "non-maskable interrupt";
	case 3:
		return "breakpoint";
	case 4:
		return "overflow";
	case 5:
		return "bound range exceeded";
	case 6:
		return "invalid opcode";
	case 7:
		return "device not available";
	case 8:
		return "double fault";
	case 10:
		return "invalid tss";
	case 11:
		return "segment not present";
	case 12:
		return "stack segment fault";
	case 13:
		return "general protection fault";
	case 14:
		return "page fault";
	case 16:
		return "x87 floating-point exception";
	case 17:
		return "alignment check";
	case 18:
		return "machine check";
	case 19:
		return "simd floating-point exception";
	default:
		return "unknown exception";
	}
}

static auto selector_table_name(u64 selector_error) -> char const *
{
	switch (selector_error & 0x3u) {
	case 0:
		return "GDT";
	case 1:
		return "IDT";
	case 2:
		return "LDT";
	default:
		return "IDT";
	}
}

static auto print_exception_details(ExceptionRegisters const &regs) -> void
{
	Debug::write_formatted("vector=0x%02x (%s) (%s)\n",
	    static_cast<u64>(regs.vector), exception_name(regs.vector),
	    exception_description(regs.vector));

	if (regs.error_code) {
		auto error { *regs.error_code };
		Debug::write_formatted("error_code=0x%016x", error);
		switch (regs.vector) {
		case 14: {
			Debug::write_formatted(
			    " (present=%d write=%d user=%d rsvd=%d instr=%d)",
			    static_cast<int>((error & 1u) != 0),
			    static_cast<int>((error & 2u) != 0),
			    static_cast<int>((error & 4u) != 0),
			    static_cast<int>((error & 8u) != 0),
			    static_cast<int>((error & 16u) != 0));
			break;
		}
		case 13:
		case 10:
		case 11:
		case 12: {
			Debug::write_formatted(" (selector=0x%04x %s index=0x%04x)",
			    static_cast<u64>(error & 0xffffu), selector_table_name(error),
			    static_cast<u64>(error >> 3));
			break;
		}
		default:
			Debug::write_formatted(" (%s)", exception_name(regs.vector));
			break;
		}
		Debug::write_formatted("\n");
	}

	if (regs.cr2)
		Debug::write_formatted("cr2=0x%016x\n", *regs.cr2);
}

static auto print_exception_registers(ExceptionRegisters const &regs) -> void
{
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

static auto is_canonical_address(u64 addr) -> bool
{
	auto const high { addr >> 48 };
	return high == 0 || high == 0xffff;
}

static auto print_backtrace_from_frame(
    u64 start_rbp, u64 start_rip, u64 stack_low, u64 stack_high) -> void
{
	constexpr u32 max_frames { 32 };

	Debug::write_formatted("Backtrace:\n");
	auto const frame_addr { start_rip };
	auto const sym0 { lookup_kernel_symbol(frame_addr) };
	Debug::write_formatted("#%02d", 0);
	if (sym0.found) {
		auto const delta { frame_addr - sym0.address };
		Debug::write_formatted(" rip=0x%016x %.*s+0x%x", frame_addr,
		    sym0.name.size(), sym0.name.data(), delta);
		if (sym0.line != 0 && sym0.file.size() != 0)
			Debug::write_formatted(
			    " at %.*s:%u", sym0.file.size(), sym0.file.data(), sym0.line);
		Debug::write_formatted("\n");
	} else {
		Debug::write_formatted(" rip=0x%016x\n", frame_addr);
	}

	auto rbp { start_rbp };
	for (u32 frame { 1 }; frame < max_frames; ++frame) {
		if (rbp == 0 || (rbp & 0x7u) != 0 || !is_canonical_address(rbp)
		    || rbp < stack_low || (rbp + 16u) > stack_high) {
			break;
		}

		auto const *current { reinterpret_cast<u64 const *>(rbp) };
		auto const next_rbp { current[0] };
		auto const return_rip { current[1] };

		if (return_rip == 0 || !is_canonical_address(return_rip)) {
			break;
		}

		auto const sym { lookup_kernel_symbol(return_rip) };
		Debug::write_formatted("#%02d", static_cast<int>(frame));
		if (sym.found) {
			auto const delta { return_rip - sym.address };
			Debug::write_formatted(" rip=0x%016x %.*s+0x%x", return_rip,
			    sym.name.size(), sym.name.data(), delta);
			if (sym.line != 0 && sym.file.size() != 0)
				Debug::write_formatted(
				    " at %.*s:%u", sym.file.size(), sym.file.data(), sym.line);
			Debug::write_formatted("\n");
		} else {
			Debug::write_formatted(" rip=0x%016x\n", return_rip);
		}

		if (next_rbp <= rbp) {
			break;
		}

		rbp = next_rbp;
	}
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

	if (vector == 14) {
		u64 cr2 {};
		asm volatile("mov %%cr2, %0" : "=r"(cr2));
		regs.cr2 = cr2;
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
	constexpr u64 stack_window { 64ull * 1024ull };

	asm volatile("cli");
	Debug::write_formatted("-=- KERNEL PANIC START -=-\n");
	Debug::write_formatted("%.*s\n\n", message.size(), message.data());
	if (regs) {
		print_exception_details(*regs);
		print_exception_registers(*regs);
		if (((*regs).cs & 0x3u) == 0) {
			Debug::write_formatted("\n");
			auto const stack_low {
				(*regs).rsp > stack_window ? (*regs).rsp - stack_window : 0ull
			};
			auto const stack_high { (*regs).rsp + stack_window };
			print_backtrace_from_frame(
			    (*regs).rbp, (*regs).rip, stack_low, stack_high);
		} else {
			Debug::write_formatted("backtrace: unavailable (userspace trap)\n");
		}
	} else {
		u64 rbp {};
		u64 rsp {};
		asm volatile("mov %%rbp, %0" : "=r"(rbp));
		asm volatile("mov %%rsp, %0" : "=r"(rsp));
		auto const rip {
			reinterpret_cast<u64>(__builtin_return_address(0)),
		};
		auto const stack_low { rsp > stack_window ? rsp - stack_window : 0ull };
		auto const stack_high { rsp + stack_window };
		print_backtrace_from_frame(rbp, rip, stack_low, stack_high);
	}
	Debug::write_formatted(
	    "\nAll wings are grounded. The heart is silent.\nSystem halted.\n");
	Debug::write_formatted("-=-  KERNEL PANIC END  -=-\n");
	Debug::drain_logs();
	panic_loop();
}

auto kpanic(CL::StringView message) -> void
{
	kpanic(message, CL::Option<ExceptionRegisters const &> {});
}

}
