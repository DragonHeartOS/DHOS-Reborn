export module Katline:FrameAllocator;

import CommonLib;
import :ArchConstants;
import :MemoryData;
import :Sync;

export {
	namespace Katline::Memory {

	class FrameAllocator {
	public:
		static auto init(MemoryMap const *mmap, uptr hhdm_offset) -> void;
		static auto reserve_phys_range(uptr phys, usize size) -> void;
		static auto allocate_page() -> void *;
		static auto allocate_zeroed_page() -> void *;
		static auto free_page(void *page) -> void;
	};

	using FA = FrameAllocator;

	}
}

namespace Katline::Memory {

static constexpr usize max_frames { 1ull << 28 };

static uptr g_hhdm_offset {};
static CL::BitArray<max_frames> g_frames;
static Katline::Sync::SpinLock g_frames_lock;

static constexpr auto page_align_down(uptr addr) -> uptr
{
	return addr & ~static_cast<uptr>(Arch::k_page_mask);
}

static constexpr auto page_align_up(uptr addr) -> uptr
{
	return (addr + Arch::k_page_size - 1)
	    & ~static_cast<uptr>(Arch::k_page_mask);
}

static constexpr auto frame_index(uptr phys) -> usize
{
	return static_cast<usize>(phys / Arch::k_page_size);
}

static auto phys_to_virt(uptr phys) -> void *
{
	return reinterpret_cast<void *>(phys + g_hhdm_offset);
}

static auto virt_to_phys(void *virt) -> uptr
{
	return reinterpret_cast<uptr>(virt) - g_hhdm_offset;
}

template<typename Fn>
static auto apply_range(uptr phys, usize size, Fn &&fn) -> void
{
	if (size == 0)
		return;

	auto const begin { page_align_down(phys) };
	auto const raw_end { phys + size };
	if (raw_end < phys)
		return;

	auto const end { page_align_up(raw_end) };
	if (end < raw_end)
		return;

	auto const first { frame_index(begin) };
	auto count { frame_index(end) - first };
	if (first >= max_frames)
		return;
	if (count > max_frames - first)
		count = max_frames - first;

	if (count == 0)
		return;

	fn(first, count);
}

static auto reserve_range(uptr phys, usize size) -> void
{
	apply_range(phys, size,
	    [](usize first, usize count) { g_frames.set_range(first, count); });
}

static auto free_range(uptr phys, usize size) -> void
{
	apply_range(phys, size,
	    [](usize first, usize count) { g_frames.clear_range(first, count); });
}

auto FrameAllocator::init(MemoryMap const *mmap, uptr hhdm_offset) -> void
{
	Katline::Sync::ScopedIrqSpinLock guard { g_frames_lock };

	g_hhdm_offset = hhdm_offset;
	g_frames.set_all();
	reserve_range(0, 0x100000);

	for (u64 i {}; i < mmap->size; ++i) {
		auto const *md { &mmap->data[i] };
		if (md->size == 0)
			continue;

		if (md->type == MemoryType::USABLE) {
			auto const phys { static_cast<uptr>(md->base) - g_hhdm_offset };
			free_range(phys, static_cast<usize>(md->size));
		}
	}
}

auto FrameAllocator::reserve_phys_range(uptr phys, usize size) -> void
{
	Katline::Sync::ScopedIrqSpinLock guard { g_frames_lock };
	reserve_range(phys, size);
}

auto FrameAllocator::allocate_page() -> void *
{
	Katline::Sync::ScopedIrqSpinLock guard { g_frames_lock };

	auto const free_frame { g_frames.find_first_clear() };
	if (!free_frame)
		return nullptr;

	auto const index { *free_frame };
	g_frames.set(index);
	auto const phys { static_cast<uptr>(index) * Arch::k_page_size };
	return phys_to_virt(phys);
}

auto FrameAllocator::allocate_zeroed_page() -> void *
{
	auto *page { allocate_page() };
	if (!page)
		return nullptr;

	__builtin_memset(page, 0, Arch::k_page_size);

	return page;
}

auto FrameAllocator::free_page(void *page) -> void
{
	if (!page)
		return;

	Katline::Sync::ScopedIrqSpinLock guard { g_frames_lock };

	auto const phys { virt_to_phys(page) };
	auto const index { frame_index(page_align_down(phys)) };
	if (index >= max_frames)
		return;

	g_frames.clear(index);
}

}
