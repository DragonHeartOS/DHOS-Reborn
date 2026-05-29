export module Katline:Paging;

import CommonLib;
import :CPU;
import :FrameAllocator;

export {
	namespace Katline::Arch::Paging {

	struct PageTable;

	enum class PageFlag : u64 {
		Present = 1ull << 0,
		Writable = 1ull << 1,
		User = 1ull << 2,
		WriteThrough = 1ull << 3,
		CacheDisable = 1ull << 4,
		Accessed = 1ull << 5,
		Dirty = 1ull << 6,
		PageSize = 1ull << 7,
		Global = 1ull << 8,
		NoExecute = 1ull << 63,
	};

	using PageFlags = CL::Flags<PageFlag>;

	struct PageTableEntry {
		u64 value {};

		constexpr auto present() const -> bool
		{
			return PageFlags { value }.contains(PageFlag::Present);
		}

		constexpr auto address() const -> uptr
		{
			return static_cast<uptr>(value & 0x000ffffffffff000ull);
		}

		constexpr auto flags() const -> u64
		{
			return value & ~0x000ffffffffff000ull;
		}

		constexpr auto clear() -> void { value = 0; }

		constexpr auto set(uptr phys, u64 flags) -> void
		{
			value = (phys & 0x000ffffffffff000ull) | flags;
		}
	};

	struct alignas(4096) PageTable {
		PageTableEntry entries[512] {};
	};

	auto init(uptr hhdm_offset) -> void;
	auto current_root() -> PageTable *;
	auto phys_address(PageTable const *table) -> uptr;
	auto create_table() -> PageTable *;
	auto map(PageTable *root, uptr virt, uptr phys, u64 flags) -> bool;
	auto map_range(
	    PageTable *root, uptr virt, uptr phys, usize pages, u64 flags) -> bool;
	auto unmap(PageTable *root, uptr virt) -> bool;
	auto translate(PageTable *root, uptr virt) -> CL::Option<uptr>;
	auto clone_address_space(PageTable const *root) -> PageTable *;
	auto clone_current_address_space() -> PageTable *;

	}
}

namespace Katline::Arch::Paging {

static uptr g_hhdm_offset {};

static constexpr uptr page_size { 4096 };
static constexpr uptr page_mask { page_size - 1 };
static constexpr uptr entry_addr_mask { 0x000ffffffffff000ull };
static constexpr u64 table_flags_mask {
	PageFlags { PageFlag::Writable, PageFlag::User }.raw(),
};

static constexpr auto pml4_index(uptr addr) -> usize
{
	return (addr >> 39) & 0x1ffu;
}
static constexpr auto pdpt_index(uptr addr) -> usize
{
	return (addr >> 30) & 0x1ffu;
}
static constexpr auto pd_index(uptr addr) -> usize
{
	return (addr >> 21) & 0x1ffu;
}
static constexpr auto pt_index(uptr addr) -> usize
{
	return (addr >> 12) & 0x1ffu;
}

static constexpr auto page_align_down(uptr addr) -> uptr
{
	return addr & ~page_mask;
}

static auto phys_to_virt(uptr phys) -> void *
{
	return reinterpret_cast<void *>(phys + g_hhdm_offset);
}

static auto virt_to_phys(void const *virt) -> uptr
{
	return reinterpret_cast<uptr>(virt) - g_hhdm_offset;
}

static auto table_from_entry(PageTableEntry const &entry) -> PageTable *
{
	return reinterpret_cast<PageTable *>(phys_to_virt(entry.address()));
}

static auto entry_flags_for_mapping(u64 flags) -> u64
{
	return (PageFlags { flags } | PageFlag::Present).raw();
}

static auto ensure_next_table(PageTableEntry &entry, u64 flags) -> PageTable *
{
	if (entry.present())
		return table_from_entry(entry);

	auto *const table { create_table() };
	if (!table)
		return nullptr;

	entry.set(virt_to_phys(table),
	    (PageFlags { flags & table_flags_mask } | PageFlag::Present).raw());

	return table;
}

struct WalkResult {
	PageTableEntry *entry {};
	usize level {}; // 1 = PT, 2 = PD, 3 = PDPT, 4 = PML4
};

static auto walk(PageTable *root, uptr virt, bool create, u64 flags)
    -> WalkResult
{
	if (!root)
		return {};

	PageTable *table { root };

	usize const indices[3] {
		pml4_index(virt),
		pdpt_index(virt),
		pd_index(virt),
	};

	usize level { 4 };
	for (usize index : indices) {
		auto &entry { table->entries[index] };

		if (create) {
			table = ensure_next_table(entry, flags);
			if (!table)
				return {};
		} else {
			if (!entry.present())
				return {};

			if (PageFlags { entry.value }.contains(PageFlag::PageSize))
				return { &entry, level - 1 };

			table = table_from_entry(entry);
		}

		--level;
	}

	return { &table->entries[pt_index(virt)], 1 };
}

static constexpr auto level_page_size(usize level) -> uptr
{
	if (level == 3)
		return 1ull << 30;
	if (level == 2)
		return 1ull << 21;
	return 1ull << 12;
}

static constexpr auto level_page_mask(usize level) -> uptr
{
	return level_page_size(level) - 1;
}

auto init(uptr hhdm_offset) -> void { g_hhdm_offset = hhdm_offset; }

auto current_root() -> PageTable *
{
	return reinterpret_cast<PageTable *>(
	    phys_to_virt(current_cr3() & ~page_mask));
}

auto phys_address(PageTable const *table) -> uptr
{
	return virt_to_phys(table);
}

auto create_table() -> PageTable *
{
	return static_cast<PageTable *>(Memory::FA::allocate_zeroed_page());
}

auto map(PageTable *root, uptr virt, uptr phys, u64 flags) -> bool
{
	auto result { walk(root, page_align_down(virt), true, flags) };
	if (!result.entry)
		return false;

	result.entry->set(page_align_down(phys), entry_flags_for_mapping(flags));
	Arch::invlpg(reinterpret_cast<void *>(page_align_down(virt)));
	return true;
}

auto map_range(PageTable *root, uptr virt, uptr phys, usize pages, u64 flags)
    -> bool
{
	if (!root)
		return false;

	for (usize i {}; i < pages; ++i) {
		auto const offset { i * page_size };
		if (!map(root, virt + offset, phys + offset, flags))
			return false;
	}

	return true;
}

auto unmap(PageTable *root, uptr virt) -> bool
{
	auto result { walk(root, page_align_down(virt), false, 0) };
	if (!result.entry || !result.entry->present() || result.level != 1)
		return false;

	result.entry->clear();
	Arch::invlpg(reinterpret_cast<void *>(page_align_down(virt)));
	return true;
}

auto translate(PageTable *root, uptr virt) -> CL::Option<uptr>
{
	auto result { walk(root, virt, false, 0) };
	if (!result.entry || !result.entry->present())
		return {};

	auto const mask { level_page_mask(result.level) };
	return (result.entry->address() & ~mask) | (virt & mask);
}

static auto clone_table_recursive(PageTable const *src, usize level)
    -> PageTable *
{
	auto *dst { create_table() };
	if (!dst)
		return nullptr;

	for (usize i {}; i < 512; ++i) {
		auto const &entry { src->entries[i] };
		if (!entry.present())
			continue;

		if (level == 1
		    || PageFlags { entry.value }.contains(PageFlag::PageSize)) {
			dst->entries[i] = entry;
			continue;
		}

		auto *const child { clone_table_recursive(
			table_from_entry(entry), level - 1) };
		if (!child)
			return nullptr;

		dst->entries[i].value = (virt_to_phys(child) & entry_addr_mask)
		    | (entry.value & ~entry_addr_mask);
	}

	return dst;
}

auto clone_address_space(PageTable const *root) -> PageTable *
{
	if (!root)
		return nullptr;

	return clone_table_recursive(root, 4);
}

auto clone_current_address_space() -> PageTable *
{
	return clone_address_space(current_root());
}

}
