#include <Katline/Katline.h>

#include <CommonLib/ArrayList.h>
#include <CommonLib/HashMap.h>
#include <CommonLib/Platform.h>
#include <CommonLib/Range.h>
#include <CommonLib/Utility.h>
#include <Katline/Arch/IDT.h>
#include <Katline/Controllers/SerialController.h>
#include <Katline/Debug.h>
#include <Katline/Memory/MemoryData.h>
#include <Katline/Memory/MemoryManager.h>

auto CL::panic(CL::StringView const message) -> void
{
	Katline::Debug::write_formatted(
	    "Kernel Panic: %.*s\n", message.size(), message.data());

	for (;;)
		asm("hlt");
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

	Memory::MM::init(info.mmap);

	IDT::init();

	CL::ArrayList<int> numbers { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	CL::ignore_unused(numbers);

	auto transformed {
		CL::range(10)
		    // numbers.iter()
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
