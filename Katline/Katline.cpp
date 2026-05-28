#include <Katline/Katline.h>

#include <CommonLib/Platform.h>
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

Controller::FramebufferController k_framebuffer_controller = NULL;
Controller::SerialController k_serial_controller;

auto katline_main(Controller::Framebuffer *framebuffer, Memory::MemoryMap *mmap)
    -> void
{
	k_serial_controller.init();
	k_framebuffer_controller = Controller::FramebufferController(framebuffer);
	Debug::set_framebuffer_logging_enabled(true);

	Memory::MM::init(mmap);

	IDT::init();
}

}
