#include <Katline/Katline.h>

#include <Katline/Arch/IDT.h>
#include <Katline/Controllers/SerialController.h>
#include <Katline/Debug.h>
#include <Katline/Memory/MemoryData.h>
#include <Katline/Memory/MemoryManager.h>

namespace Katline {

Controller::FramebufferController k_framebuffer_controller = NULL;
Controller::SerialController k_serial_controller;

void KatlineMain(Controller::Framebuffer* framebuffer, Memory::MemoryMap* mmap)
{
    k_serial_controller.Init();

    Memory::MM::Init(mmap);

    IDT::Init();

    k_framebuffer_controller = Controller::FramebufferController(framebuffer);
    Debug::SetFramebufferLoggingEnabled(true);
}

}
