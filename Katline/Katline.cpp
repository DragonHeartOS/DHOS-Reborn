#include <Katline/Katline.h>

#include <CommonLib/ArrayList.h>
#include <CommonLib/Platform.h>
#include <CommonLib/Range.h>
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

auto katline_main(Controller::Framebuffer *framebuffer, Memory::MemoryMap *mmap)
    -> void
{
	k_serial_controller.init();
	k_framebuffer_controller = Controller::FramebufferController(framebuffer);
	Debug::set_framebuffer_logging_enabled(true);

	Memory::MM::init(mmap);

	IDT::init();

	auto transformed {
		CL::Range(0, 10)
		    .map([](int value) { return value * 2; })
		    .filter([](int value) { return value >= 6; })
		    .rev()
		    .collect<CL::ArrayList<int>>(),
	};

	Debug::print_formatted("[demo] arraylist: ");
	transformed.iter().for_each(
	    [](int value) { Debug::print_formatted("%d ", value); });
	Debug::print_formatted("\n");
}

}
