export module Katline:Paging;

import CommonLib;
import :CPU;
import :ArchConstants;
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

	struct PageLookup {
		uptr physical {};
		PageFlags flags {};
		uptr page_size {};
	};

	struct PageTableEntry {
		static constexpr u64 address_mask { 0x000ffffffffff000ull };

		u64 value {};

		constexpr auto present() const -> bool
		{
			return PageFlags { value }.contains(PageFlag::Present);
		}

		constexpr auto address() const -> uptr
		{
			return static_cast<uptr>(value & address_mask);
		}

		constexpr auto flags() const -> PageFlags
		{
			return PageFlags { value & ~address_mask };
		}

		constexpr auto clear() -> void { value = 0; }

		constexpr auto set(uptr phys, PageFlags flags) -> void
		{
			value = (phys & address_mask) | flags.raw();
		}
	};

	struct alignas(k_page_size) PageTable {
		PageTableEntry entries[512] {};
	};

	auto init(uptr hhdm_offset) -> void;
	auto current_root() -> PageTable *;
	auto phys_address(PageTable const *table) -> uptr;
	auto phys_to_virt(uptr phys) -> void *;
	auto virt_to_phys(void const *virt) -> uptr;
	auto create_table() -> PageTable *;
	auto clone_kernel_address_space() -> PageTable *;

	auto map(PageTable *root, uptr virt, uptr phys, PageFlags flags) -> bool;

	auto map_range(PageTable *root, uptr virt, uptr phys, usize pages,
	    PageFlags flags) -> bool;

	auto unmap(PageTable *root, uptr virt) -> bool;
	auto translate(PageTable *root, uptr virt) -> CL::Option<uptr>;
	auto query(PageTable *root, uptr virt) -> CL::Option<PageLookup>;

	auto clone_address_space(PageTable const *root) -> PageTable *;
	auto clone_current_address_space() -> PageTable *;

	}
}

namespace Katline::Arch::Paging {

static uptr g_hhdm_offset {};

static constexpr uptr page_mask { k_page_mask };

static constexpr PageFlags table_flags_mask {
	PageFlag::Writable,
	PageFlag::User,
};

static constexpr auto page_align_down(uptr addr) -> uptr
{
	return addr & ~page_mask;
}

static constexpr auto table_index(uptr addr, usize level) -> usize
{
	return (addr >> (12 + 9 * (level - 1))) & 0x1ffu;
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

auto phys_to_virt(uptr phys) -> void *
{
	return reinterpret_cast<void *>(phys + g_hhdm_offset);
}

auto virt_to_phys(void const *virt) -> uptr
{
	return reinterpret_cast<uptr>(virt) - g_hhdm_offset;
}

static auto table_from_entry(PageTableEntry const &entry) -> PageTable *
{
	return reinterpret_cast<PageTable *>(phys_to_virt(entry.address()));
}

static auto mapping_flags(PageFlags flags) -> PageFlags
{
	return flags | PageFlag::Present;
}

static auto table_flags(PageFlags flags) -> PageFlags
{
	return (flags & table_flags_mask) | PageFlag::Present;
}

static auto ensure_next_table(PageTableEntry &entry, PageFlags flags)
    -> PageTable *
{
	if (entry.present())
		return table_from_entry(entry);

	auto *const table { create_table() };
	if (!table)
		return nullptr;

	entry.set(virt_to_phys(table), table_flags(flags));
	return table;
}

struct WalkResult {
	PageTableEntry *entry {};
	usize level {};
};

static auto walk(PageTable *root, uptr virt, bool create, PageFlags flags)
    -> WalkResult
{
	if (!root)
		return {};

	auto *table { root };

	for (usize level { 4 }; level > 1; --level) {
		auto &entry { table->entries[table_index(virt, level)] };

		if (!entry.present()) {
			if (!create)
				return {};

			table = ensure_next_table(entry, flags);
			if (!table)
				return {};

			continue;
		}

		if (entry.flags().contains(PageFlag::PageSize))
			return { &entry, level - 1 };

		table = table_from_entry(entry);
	}

	return { &table->entries[table_index(virt, 1)], 1 };
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

auto map(PageTable *root, uptr virt, uptr phys, PageFlags flags) -> bool
{
	auto const aligned_virt { page_align_down(virt) };

	auto result { walk(root, aligned_virt, true, flags) };
	if (!result.entry)
		return false;

	result.entry->set(page_align_down(phys), mapping_flags(flags));
	Arch::invlpg(reinterpret_cast<void *>(aligned_virt));

	return true;
}

auto map_range(
    PageTable *root, uptr virt, uptr phys, usize pages, PageFlags flags) -> bool
{
	if (!root)
		return false;

	for (usize i {}; i < pages; ++i) {
		auto const offset { i * k_page_size };

		if (!map(root, virt + offset, phys + offset, flags))
			return false;
	}

	return true;
}

auto unmap(PageTable *root, uptr virt) -> bool
{
	auto const aligned_virt { page_align_down(virt) };

	auto result { walk(root, aligned_virt, false, {}) };
	if (!result.entry || !result.entry->present() || result.level != 1)
		return false;

	result.entry->clear();
	Arch::invlpg(reinterpret_cast<void *>(aligned_virt));

	return true;
}

auto translate(PageTable *root, uptr virt) -> CL::Option<uptr>
{
	auto result { walk(root, virt, false, {}) };
	if (!result.entry || !result.entry->present())
		return {};

	auto const mask { level_page_mask(result.level) };

	return (result.entry->address() & ~mask) | (virt & mask);
}

auto query(PageTable *root, uptr virt) -> CL::Option<PageLookup>
{
	auto result { walk(root, virt, false, {}) };
	if (!result.entry || !result.entry->present())
		return {};

	auto const mask { level_page_mask(result.level) };
	return PageLookup {
		.physical = (result.entry->address() & ~mask) | (virt & mask),
		.flags = result.entry->flags(),
		.page_size = level_page_size(result.level),
	};
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

		if (level == 1 || entry.flags().contains(PageFlag::PageSize)) {
			dst->entries[i] = entry;
			continue;
		}

		auto *const child { clone_table_recursive(
			table_from_entry(entry), level - 1) };

		if (!child)
			return nullptr;

		dst->entries[i].set(virt_to_phys(child), entry.flags());
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

auto clone_kernel_address_space() -> PageTable *
{
	auto *const current { current_root() };
	if (!current)
		return nullptr;

	auto *root { create_table() };
	if (!root)
		return nullptr;

	for (usize i { 256 }; i < 512; ++i)
		root->entries[i] = current->entries[i];

	return root;
}

}
