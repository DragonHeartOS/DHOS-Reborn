export module Katline:Runtime;

import CommonLib;
import :FramebufferController;
import :SerialController;
import :MemoryData;
import :IDT;
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

	IDT::init();
	k_framebuffer_controller
	    = Controller::FramebufferController(info.framebuffer);

	Debug::set_framebuffer_logging_enabled(true);

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
}

}
