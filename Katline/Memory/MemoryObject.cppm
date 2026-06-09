export module Katline:MemoryObject;

import CommonLib;
import :ArchConstants;
import :FrameAllocator;
import :Paging;

export {
	namespace Katline::Memory {

	enum class MemoryObjectKind : u8 {
		Allocated,
		MMIO,
	};

	struct MemoryObject {
		~MemoryObject();

		MemoryObjectKind kind {};
		usize size {};
		CL::ArrayList<uptr> pages {};
		uptr phys_base {};
		u64 ref_count {};

		static auto create_allocated(usize size) -> MemoryObject *;
		static auto create_mmio(uptr phys_base, usize size) -> MemoryObject *;
	};

	auto retain_memory_object(MemoryObject *object) -> void;
	auto release_memory_object(MemoryObject *object) -> void;

	}
}

namespace Katline::Memory {

static constexpr auto page_align_up(uptr value) -> uptr
{
	auto const aligned {
		(value + Arch::k_page_size - 1) & ~static_cast<uptr>(Arch::k_page_mask),
	};
	return aligned;
}

auto retain_memory_object(MemoryObject *object) -> void
{
	if (object)
		object->ref_count++;
}

auto release_memory_object(MemoryObject *object) -> void
{
	if (!object || object->ref_count == 0)
		return;

	if (--object->ref_count == 0)
		delete object;
}

MemoryObject::~MemoryObject()
{
	if (kind == MemoryObjectKind::Allocated) {
		for (usize i {}; i < pages.size(); ++i) {
			auto const phys { *pages.get(i) };
			Memory::FA::free_page(Arch::Paging::phys_to_virt(phys));
		}
	}

	pages.clear();
}

auto MemoryObject::create_allocated(usize size) -> MemoryObject *
{
	if (size == 0)
		return nullptr;

	auto *object { new MemoryObject {} };
	object->kind = MemoryObjectKind::Allocated;
	object->size = size;
	object->ref_count = 0;

	auto const page_count { page_align_up(size) / Arch::k_page_size };
	for (usize i {}; i < page_count; ++i) {
		auto *page { Memory::FA::allocate_zeroed_page() };
		if (!page) {
			delete object;
			return nullptr;
		}

		object->pages.push(Arch::Paging::virt_to_phys(page));
	}

	return object;
}

auto MemoryObject::create_mmio(uptr phys_base, usize size) -> MemoryObject *
{
	if (size == 0 || (phys_base & (Arch::k_page_size - 1)) != 0)
		return nullptr;

	auto *object { new MemoryObject {} };
	object->kind = MemoryObjectKind::MMIO;
	object->size = size;
	object->phys_base = phys_base;
	object->ref_count = 0;

	return object;
}

}
