export module Katline:X2APIC;

import CommonLib;
import :CPU;
import :Interrupts;
import :IO;
import :Scheduler;

export {
	namespace Katline::Arch::X2APIC {

	namespace ErrorsV {
	struct ModeNotLatched { };
	struct SvrNotEnabled { };
	struct CalibrationFailed { };
	struct LvtReadbackMismatch { };
	}

	using Errors = CL::Error<ErrorsV::ModeNotLatched, ErrorsV::SvrNotEnabled,
	    ErrorsV::CalibrationFailed, ErrorsV::LvtReadbackMismatch>;

	auto init_timer(u64 tsc_frequency_hz) -> CL::Result<void, Errors>;
	auto error_to_string(Errors const &error) -> CL::StringView;
	auto timer_ticks() -> u64;

	}
}

namespace Katline::Debug {
void print_formatted(char const *str, ...);
}

namespace Katline {
struct interrupt_frame;
}

namespace Katline::Arch::X2APIC::ErrorsV {

inline auto to_display_string(ModeNotLatched const &) -> CL::String
{
	return "x2APIC mode not latched";
}

inline auto to_display_string(SvrNotEnabled const &) -> CL::String
{
	return "x2APIC SVR software-enable failed";
}

inline auto to_display_string(CalibrationFailed const &) -> CL::String
{
	return "x2APIC timer calibration failed";
}

inline auto to_display_string(LvtReadbackMismatch const &) -> CL::String
{
	return "x2APIC timer register readback mismatch";
}

}

namespace Katline::Arch::X2APIC {

static constexpr u8 timer_vector = 0x40;

static constexpr u32 ia32_apic_base_msr = 0x1b;
static constexpr u64 ia32_apic_base_enable = 1ull << 11;
static constexpr u64 ia32_apic_base_x2apic_enable = 1ull << 10;

static constexpr u32 x2apic_msr_base = 0x800;
static constexpr u32 reg_tpr = 0x08;
static constexpr u32 reg_eoi = 0x0b;
static constexpr u32 reg_svr = 0x0f;
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

template<typename E> static auto fail(E error) -> CL::Result<void, Errors>
{
	return CL::Result<void, Errors>::Err(Errors { CL::forward<E>(error) });
}

extern "C" [[gnu::interrupt]] auto x2apic_timer_interrupt_handler(
    interrupt_frame *) -> void
{
	g_timer_ticks = g_timer_ticks + 1;
	x2apic_write(reg_eoi, 0);
	Scheduler::the().on_timer_tick();
}

static auto enable_x2apic() -> void
{
	CPUIDRegs::serialize();
	auto apic_base { rdmsr(ia32_apic_base_msr) };
	apic_base |= ia32_apic_base_enable;
	apic_base |= ia32_apic_base_x2apic_enable;
	wrmsr(ia32_apic_base_msr, apic_base);
	CPUIDRegs::serialize();
}

static auto x2apic_mode_latched() -> bool
{
	auto apic_base { rdmsr(ia32_apic_base_msr) };
	bool apic_enabled = (apic_base & ia32_apic_base_enable) != 0;
	bool x2apic_enabled = (apic_base & ia32_apic_base_x2apic_enable) != 0;
	return apic_enabled && x2apic_enabled;
}

static auto mask_legacy_pic() -> void
{
	IO::outb(0x21, 0xff);
	IO::outb(0xa1, 0xff);
}

static auto measure_apic_ticks_per_second_once(u64 tsc_frequency_hz) -> u64
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

	CPUIDRegs::serialize();
	u64 tsc_start { rdtscp() };
	for (;;) {
		u64 now { rdtscp() };
		if (now - tsc_start >= wait_cycles)
			break;
		asm volatile("pause");
	}
	CPUIDRegs::serialize();

	u32 cur_count { x2apic_read(reg_timer_current_count) };
	u64 elapsed_apic_ticks = 0xffffffffull - static_cast<u64>(cur_count);
	if (!elapsed_apic_ticks)
		return 0;

	return elapsed_apic_ticks * (1000 / calibration_ms);
}

static auto calibrate_apic_ticks_per_second(u64 tsc_frequency_hz) -> u64
{
	u64 s0 { measure_apic_ticks_per_second_once(tsc_frequency_hz) };
	u64 s1 { measure_apic_ticks_per_second_once(tsc_frequency_hz) };
	u64 s2 { measure_apic_ticks_per_second_once(tsc_frequency_hz) };

	if (!s0 || !s1 || !s2)
		return 0;

	auto median { CL::Math::median3(s0, s1, s2) };
	Debug::print_formatted("[x2APIC] cal samples=%d,%d,%d median=%d\n",
	    static_cast<int>(s0), static_cast<int>(s1), static_cast<int>(s2),
	    static_cast<int>(median));
	return median;
}

static auto validate_timer_readback(u32 reload) -> bool
{
	auto svr { x2apic_read(reg_svr) };
	if ((svr & svr_software_enable) == 0)
		return false;

	auto lvt { x2apic_read(reg_lvt_timer) };
	auto divide { x2apic_read(reg_timer_divide) };
	bool vector_ok = (lvt & 0xffu) == timer_vector;
	bool periodic_ok = (lvt & lvt_timer_periodic) != 0;
	bool unmasked = (lvt & lvt_masked) == 0;
	bool divide_ok = (divide & 0x0fu) == 0x3u;
	bool reload_ok = x2apic_read(reg_timer_initial_count) == reload;
	return vector_ok && periodic_ok && unmasked && divide_ok && reload_ok;
}

auto init_timer(u64 tsc_frequency_hz) -> CL::Result<void, Errors>
{
	enable_x2apic();
	if (!x2apic_mode_latched())
		return fail(ErrorsV::ModeNotLatched {});

	mask_legacy_pic();
	x2apic_write(reg_tpr, 0);
	x2apic_write(reg_svr, svr_software_enable | 0xffu);
	if ((x2apic_read(reg_svr) & svr_software_enable) == 0)
		return fail(ErrorsV::SvrNotEnabled {});

	Interrupts::register_isr(timer_vector,
	    reinterpret_cast<void (*)()>(&x2apic_timer_interrupt_handler));

	auto apic_ticks_per_second { calibrate_apic_ticks_per_second(
		tsc_frequency_hz) };
	if (!apic_ticks_per_second)
		return fail(ErrorsV::CalibrationFailed {});

	u32 reload = static_cast<u32>(apic_ticks_per_second / 250ull);
	if (!reload)
		reload = 1;

	x2apic_write(reg_timer_divide, 0x3);
	x2apic_write(reg_lvt_timer, lvt_timer_periodic | timer_vector);
	x2apic_write(reg_timer_initial_count, reload);

	if (!validate_timer_readback(reload))
		return fail(ErrorsV::LvtReadbackMismatch {});

	Debug::print_formatted("[x2APIC] timer=%dHz ticks_per_sec=%d reload=%d\n",
	    250, static_cast<int>(apic_ticks_per_second), static_cast<int>(reload));

	return CL::Result<void, Errors>::Ok();
}

auto error_to_string(Errors const &error) -> CL::StringView
{
	if (error.template get<ErrorsV::ModeNotLatched>().is_some())
		return "x2APIC mode not latched";
	if (error.template get<ErrorsV::SvrNotEnabled>().is_some())
		return "x2APIC SVR software-enable failed";
	if (error.template get<ErrorsV::CalibrationFailed>().is_some())
		return "x2APIC timer calibration failed";
	if (error.template get<ErrorsV::LvtReadbackMismatch>().is_some())
		return "x2APIC timer register readback mismatch";
	return "x2APIC unknown error";
}

auto timer_ticks() -> u64 { return g_timer_ticks; }

}
