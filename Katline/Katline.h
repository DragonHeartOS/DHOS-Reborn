#pragma once

#include <CommonLib/Span.h>
#include <Katline/Controllers/FramebufferController.h>
#include <Katline/Controllers/SerialController.h>
#include <Katline/Memory/MemoryData.h>

namespace Katline {

struct StartupInfo {
	struct MPInfo {
		u32 processor_id;
		u32 lapic_id;

	private:
		[[maybe_unused]] u64 _;

	public:
		uintptr_t goto_address;
		u64 extra;
	};

	Controller::Framebuffer *framebuffer;
	Memory::MemoryMap *mmap;
	u32 bsp_lapic_id;
	CL::Span<MPInfo *> mp_info;
	uintptr_t rsdp_address;
	uintptr_t hhdm_offset;
};

extern Controller::FramebufferController k_framebuffer_controller;
extern Controller::SerialController k_serial_controller;

void katline_main(StartupInfo &info);

}
