module;

#include <BootstrapEntry.h>

#ifndef KATLINE_BOOTSTRAP_BIN_PATH
#	error "KATLINE_BOOTSTRAP_BIN_PATH must be defined"
#endif

export module Katline:Bootstrap;

import CommonLib;
import :ArchConstants;
import :Debug;
import :FrameAllocator;
import :Paging;
import :Panic;
import :Scheduler;

static constexpr uptr bootstrap_image_base { 0x0000000000400000ull };
static constexpr usize bootstrap_stack_size { 0x4000 };
static constexpr uptr bootstrap_stack_top { 0x00007fffffffe000ull };

static unsigned char const bootstrap_image[] = {
#embed KATLINE_BOOTSTRAP_BIN_PATH
};

static auto map_bootstrap_region(Katline::Arch::Paging::PageTable *root,
    uptr virt_base, unsigned char const *data, usize size,
    Katline::Arch::Paging::PageFlags flags) -> bool
{
	if (size == 0)
		return true;

	auto const page_count {
		(size + Katline::Arch::k_page_size - 1) / Katline::Arch::k_page_size,
	};

	for (usize i {}; i < page_count; ++i) {
		auto *page { Katline::Memory::FA::allocate_zeroed_page() };
		if (!page)
			return false;

		auto const phys { Katline::Arch::Paging::virt_to_phys(page) };
		auto const offset { i * Katline::Arch::k_page_size };
		auto const remaining { size - offset };
		auto const chunk {
			remaining < Katline::Arch::k_page_size ? remaining
			                                       : Katline::Arch::k_page_size,
		};

		CL::memcpy(page, data + offset, chunk);

		if (!Katline::Arch::Paging::map(root, virt_base + offset, phys, flags))
			return false;
	}

	return true;
}

export namespace Katline {

auto launch_bootstrap_process() -> void
{
	using namespace Arch::Paging;

	auto *process { Arch::Scheduler::the().create_user_process(
		Arch::k_process) };
	if (!process)
		kpanic("[Bootstrap] failed to create bootstrap process");

	process->capabilities = ProcessCapabilityFlags::all();

	process->name = CL::String { "Bootstrap" };

	auto *root { reinterpret_cast<PageTable *>(phys_to_virt(process->cr3)) };
	if (!root)
		kpanic("[Bootstrap] invalid page table");

	if (!map_bootstrap_region(root, bootstrap_image_base, bootstrap_image,
	        sizeof(bootstrap_image), { PageFlag::User, PageFlag::Writable }))
		kpanic("[Bootstrap] failed to map bootstrap image");

	auto const stack_base { bootstrap_stack_top - bootstrap_stack_size };
	auto stack_page_count { bootstrap_stack_size / Katline::Arch::k_page_size };
	for (usize i {}; i < stack_page_count; ++i) {
		auto *page { Katline::Memory::FA::allocate_zeroed_page() };
		if (!page)
			kpanic("[Bootstrap] failed to allocate stack page");

		auto const phys { virt_to_phys(page) };
		auto const virt { stack_base + i * Katline::Arch::k_page_size };
		if (!map(root, virt, phys, { PageFlag::User, PageFlag::Writable }))
			kpanic("[Bootstrap] failed to map bootstrap stack page");
	}

	auto *thread {
		Arch::Scheduler::the().make_user_thread(
		    process, KATLINE_BOOTSTRAP_ENTRY_ADDRESS, bootstrap_stack_top),
	};
	if (!thread)
		kpanic("[Bootstrap] failed to create bootstrap thread");

	if (!Arch::Scheduler::the().start_thread(thread))
		kpanic("[Bootstrap] failed to start bootstrap thread");

	Debug::print_formatted("[Bootstrap] launched pid=%d tid=%d\n",
	    process->pid.id, thread->tid.id);
	Debug::drain_logs();
}

}
