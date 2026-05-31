export module Katline:SyscallABI;

import CommonLib;
import :CPU;
import :GDT;

export {
	namespace Katline::Syscalls {

	struct RawArguments {
		u64 number {};
		u64 arg0 {};
		u64 arg1 {};
		u64 arg2 {};
		u64 arg3 {};
		u64 arg4 {};
	};

	auto init() -> void;
	extern "C" auto syscall_entry() -> void;
	extern "C" auto dispatch_raw(
	    u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) -> u64;

	}
}

namespace Katline::Syscalls {

static constexpr u32 ia32_efer { 0xc0000080 };
static constexpr u32 ia32_star { 0xc0000081 };
static constexpr u32 ia32_lstar { 0xc0000082 };
static constexpr u32 ia32_fmask { 0xc0000084 };
static constexpr u64 efer_syscall_enable { 1ull << 0 };

static constexpr u16 kernel_code_selector { 0x08 };
static constexpr u16 user_code_selector { 0x1b };

auto init() -> void
{
	auto efer { Arch::rdmsr(ia32_efer) };
	efer |= efer_syscall_enable;
	Arch::wrmsr(ia32_efer, efer);

	auto const star {
		(static_cast<u64>(user_code_selector - 16) << 48)
		    | (static_cast<u64>(kernel_code_selector) << 32),
	};
	Arch::wrmsr(ia32_star, star);
	Arch::wrmsr(ia32_lstar, reinterpret_cast<u64>(&syscall_entry));

	// Clear TF, DF, IF, AC, and NT.
	Arch::wrmsr(ia32_fmask,
	    (1ull << 8) | (1ull << 9) | (1ull << 10) | (1ull << 14) | (1ull << 18));
}

extern "C" [[gnu::naked]] auto syscall_entry() -> void
{
	asm volatile("movq %rcx, %r12\n\t"
	             "movq %r11, %r13\n\t"
	             "movq %rsp, %r14\n\t"
	             "movq %rax, %r15\n\t"
	             "movq %rdi, %rbx\n\t"
	             "movq %rsi, %rbp\n\t"
	             "callq katline_current_kernel_stack_top\n\t"
	             "testq %rax, %rax\n\t"
	             "jz 1f\n\t"
	             "movq %rax, %rsp\n\t"
	             "movq %r15, %rdi\n\t"
	             "movq %rbx, %rsi\n\t"
	             "movq %rbp, %rdx\n\t"
	             "xorq %rcx, %rcx\n\t"
	             "xorq %r8, %r8\n\t"
	             "xorq %r9, %r9\n\t"
	             "callq dispatch_raw\n\t"
	             "movq %r12, %rcx\n\t"
	             "movq %r13, %r11\n\t"
	             "movq %r14, %rsp\n\t"
	             "sysretq\n\t"
	             "1:\n\t"
	             "cli\n\t"
	             "hlt\n\t"
	             "jmp 1b\n\t");
}

}
