#pragma once

#include <Katline/Controllers/FramebufferController.h>
#include <Katline/Controllers/SerialController.h>
#include <Katline/Memory/MemoryData.h>

namespace Katline {

extern Controller::FramebufferController k_framebuffer_controller;
extern Controller::SerialController k_serial_controller;

void KatlineMain(Controller::Framebuffer *framebuffer, Memory::MemoryMap *mmap);

}
