export module Katline:Runtime;

import CommonLib;
import :FramebufferController;
import :SerialController;
import :MemoryData;
import :CPU;
import :Interrupts;
import :X2APIC;
import :MemoryManager;

export {
	namespace Katline {

	struct StartupInfo {
		struct MPInfo {
			u32 processor_id;
			u32 lapic_id;

		private:
			[[maybe_unused]] u64 _;

		public:
			uptr goto_address;
			u64 extra;
		};

		Controller::Framebuffer *framebuffer;
		Memory::MemoryMap *mmap;
		u32 bsp_lapic_id;
		CL::Span<MPInfo *> mp_info;
		uptr rsdp_address;
		uptr hhdm_offset;
		u64 tsc_frequency_hz;
	};

	extern Controller::FramebufferController k_framebuffer_controller;
	extern Controller::SerialController k_serial_controller;

	void katline_main(StartupInfo &info);

	}
}

namespace Katline::Debug {
void set_framebuffer_logging_enabled(bool enabled);
void print_formatted(char const *str, ...);
}

namespace Katline {

Controller::FramebufferController k_framebuffer_controller { nullptr };
Controller::SerialController k_serial_controller;

auto katline_main(StartupInfo &info) -> void
{
	k_serial_controller.init();
	k_framebuffer_controller
	    = Controller::FramebufferController(info.framebuffer);
	Debug::set_framebuffer_logging_enabled(true);

	auto cpuid_info { Arch::CPUID::query_cpuid() };
	Debug::print_formatted(
	    "[CPU] vendor=%s family=%d model=%d stepping=%d apic=%d x2apic=%d\n",
	    cpuid_info.vendor_id,
	    static_cast<int>(cpuid_info.family + cpuid_info.ext_family),
	    static_cast<int>((cpuid_info.ext_model << 4) | cpuid_info.model),
	    static_cast<int>(cpuid_info.stepping_id),
	    static_cast<int>(cpuid_info.has_apic),
	    static_cast<int>(cpuid_info.has_x2apic));

	if (!cpuid_info.has_apic || !cpuid_info.has_x2apic) {
		Debug::print_formatted(
		    "[CPU] missing APIC/x2APIC support; halting for bring-up.\n");
		for (;;)
			asm("hlt");
	}

	Interrupts::init_defaults();
	auto timer_init_result { Arch::X2APIC::init_timer(info.tsc_frequency_hz) };
	if (timer_init_result.is_err()) {
		auto const &error { timer_init_result.unwrap_err() };
		Debug::print_formatted("[x2APIC] timer init failed: %s\n",
		    Arch::X2APIC::error_to_string(error).data());
		for (;;)
			asm("hlt");
	}
	asm volatile("sti");

	Memory::MM::init(info.mmap);

	CL::ArrayList<int> numbers { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	CL::ignore_unused(numbers);

	auto transformed {
		CL::range(10)
		    .map([](int value) { return value * 2; })
		    .filter([](int value) { return value >= 6; })
		    .rev()
		    .collect<CL::ArrayList<int>>(),
	};

	Debug::print_formatted("[demo] arraylist: ");
	transformed.iter().for_each(
	    [](int value) { Debug::print_formatted("%d ", value); });
	Debug::print_formatted("\n");

	CL::HashMap<int, int> squares;
	squares.insert_or_replace(2, 4);
	squares.insert_or_replace(3, 9);

	Debug::print_formatted("[demo] hashmap: ");
	squares.iter().for_each([](auto const &entry) {
		Debug::print_formatted("(%d->%d) ", entry.key, entry.value);
	});
	Debug::print_formatted("\n");

	u64 last_logged {};
	for (;;) {
		u64 ticks { Arch::X2APIC::timer_ticks() };
		u64 cur { ticks / 250ull };
		if (cur != 0 && cur != last_logged) {
			last_logged = cur;
			Debug::print_formatted("[x2APIC] tick=%d (~%ds)\n",
			    static_cast<int>(ticks), static_cast<int>(cur));
		}
		asm("hlt");
	}
}

}
