export module Katline:SyscallABI;

import CommonLib;
import :CPU;
import :GDT;

export {
	namespace Katline::Syscalls {

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

auto init() -> void
{
	auto efer { Arch::rdmsr(ia32_efer) };
	efer |= efer_syscall_enable;
	Arch::wrmsr(ia32_efer, efer);

	auto const star {
		(static_cast<u64>(Arch::user_code_selector - 16) << 48)
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
	asm volatile("swapgs\n\t"
	             "movq %rsp, %gs:8\n\t"
	             "movq %gs:0, %rsp\n\t"
	             "testq %rsp, %rsp\n\t"
	             "jz 1f\n\t"
	             "andq $-16, %rsp\n\t"

	             "pushq %rbx\n\t"
	             "pushq %rbp\n\t"
	             "pushq %r12\n\t"
	             "pushq %r13\n\t"
	             "pushq %r14\n\t"
	             "pushq %r15\n\t"
	             "pushq %rcx\n\t"
	             "pushq %r11\n\t"

	             "movq %r8, %r9\n\t"
	             "movq %r10, %r8\n\t"
	             "movq %rdx, %rcx\n\t"
	             "movq %rsi, %rdx\n\t"
	             "movq %rdi, %rsi\n\t"
	             "movq %rax, %rdi\n\t"
	             "callq dispatch_raw\n\t"

	             "popq %r11\n\t"
	             "popq %rcx\n\t"
	             "popq %r15\n\t"
	             "popq %r14\n\t"
	             "popq %r13\n\t"
	             "popq %r12\n\t"
	             "popq %rbp\n\t"
	             "popq %rbx\n\t"

	             "movq %gs:8, %rsp\n\t"
	             "swapgs\n\t"
	             "sysretq\n\t"
	             "1:\n\t"
	             "swapgs\n\t"
	             "cli\n\t"
	             "hlt\n\t"
	             "jmp 1b\n\t");
}
}
