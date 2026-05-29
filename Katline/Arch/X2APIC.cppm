export module Katline:X2APIC;

import CommonLib;
import :CPU;
import :IDT;
import :IO;

export {
	namespace Katline::Arch::X2APIC {

	auto init_timer(u64 tsc_frequency_hz) -> bool;
	auto timer_ticks() -> u64;

	}
}

namespace Katline::Debug {
void print_formatted(char const *str, ...);
}

namespace Katline {
struct interrupt_frame;
}

namespace Katline::Arch::X2APIC {

static constexpr u8 timer_vector = 0x40;

static constexpr u32 ia32_apic_base_msr = 0x1b;
static constexpr u64 ia32_apic_base_enable = 1ull << 11;
static constexpr u64 ia32_apic_base_x2apic_enable = 1ull << 10;

static constexpr u32 x2apic_msr_base = 0x800;
static constexpr u32 reg_svr = 0x0f;
static constexpr u32 reg_tpr = 0x08;
static constexpr u32 reg_eoi = 0x0b;
static constexpr u32 reg_lvt_timer = 0x32;
static constexpr u32 reg_timer_initial_count = 0x38;
static constexpr u32 reg_timer_current_count = 0x39;
static constexpr u32 reg_timer_divide = 0x3e;

static constexpr u32 lvt_timer_periodic = 1u << 17;
static constexpr u32 lvt_masked = 1u << 16;
static constexpr u32 svr_software_enable = 1u << 8;

static u64 volatile g_timer_ticks {};

static auto x2apic_msr(u32 reg) -> u32 { return x2apic_msr_base + reg; }

static auto x2apic_write(u32 reg, u32 value) -> void
{
	wrmsr(x2apic_msr(reg), value);
}

static auto x2apic_read(u32 reg) -> u32
{
	return static_cast<u32>(rdmsr(x2apic_msr(reg)) & 0xffffffffull);
}

extern "C" [[gnu::interrupt]] auto x2apic_timer_interrupt_handler(
    interrupt_frame *) -> void
{
	g_timer_ticks = g_timer_ticks + 1;
	x2apic_write(reg_eoi, 0);
}

static auto enable_x2apic() -> void
{
	auto apic_base { rdmsr(ia32_apic_base_msr) };
	apic_base |= ia32_apic_base_enable;
	apic_base |= ia32_apic_base_x2apic_enable;
	wrmsr(ia32_apic_base_msr, apic_base);
}

static auto mask_legacy_pic() -> void
{
	IO::outb(0x21, 0xff);
	IO::outb(0xa1, 0xff);
}

static auto calibrate_apic_ticks_per_second(u64 tsc_frequency_hz) -> u64
{
	if (!tsc_frequency_hz)
		return 0;

	constexpr u64 calibration_ms = 50;
	u64 wait_cycles = (tsc_frequency_hz * calibration_ms) / 1000;
	if (!wait_cycles)
		wait_cycles = 1;

	x2apic_write(reg_timer_divide, 0x3);
	x2apic_write(reg_lvt_timer, lvt_masked | timer_vector);
	x2apic_write(reg_timer_initial_count, 0xffffffffu);

	u64 tsc_start { rdtsc() };
	for (;;) {
		u64 now { rdtsc() };
		if (now - tsc_start >= wait_cycles)
			break;
		asm volatile("pause");
	}

	u32 cur_count { x2apic_read(reg_timer_current_count) };
	u64 elapsed_apic_ticks = 0xffffffffull - static_cast<u64>(cur_count);
	if (!elapsed_apic_ticks)
		return 0;

	return elapsed_apic_ticks * (1000 / calibration_ms);
}

auto init_timer(u64 tsc_frequency_hz) -> bool
{
	enable_x2apic();
	mask_legacy_pic();
	x2apic_write(reg_tpr, 0);
	x2apic_write(reg_svr, svr_software_enable | 0xffu);

	IDT::set_handler(timer_vector,
	    reinterpret_cast<void (*)()>(&x2apic_timer_interrupt_handler));

	auto apic_ticks_per_second { calibrate_apic_ticks_per_second(
		tsc_frequency_hz) };
	if (!apic_ticks_per_second)
		return false;

	u32 reload = static_cast<u32>(apic_ticks_per_second / 250ull);
	if (!reload)
		reload = 1;

	x2apic_write(reg_timer_divide, 0x3);
	x2apic_write(reg_lvt_timer, lvt_timer_periodic | timer_vector);
	x2apic_write(reg_timer_initial_count, reload);

	Debug::print_formatted("[x2APIC] timer=%dHz ticks_per_sec=%d reload=%d\n",
	    250, static_cast<int>(apic_ticks_per_second), static_cast<int>(reload));

	return true;
}

auto timer_ticks() -> u64 { return g_timer_ticks; }

}
