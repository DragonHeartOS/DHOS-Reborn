export module Katline:SyscallMemoryMapHelpers;

import CommonLib;
import KatlineAPI;
import :ArchConstants;
import :MemoryObject;
import :Paging;
import :ProcessMappings;
import :Scheduler;
import :SyscallKernelContract;

namespace Katline::Syscalls {

static constexpr Katline::MemoryMapFlags k_memory_map_allowed_flags {
	Katline::MemoryMapFlag::Writable,
	Katline::MemoryMapFlag::NoExecute,
};

}

export {
	namespace Katline::Syscalls {

	auto required_page_count(u64 offset, u64 size) -> usize;
	auto page_flags_from_memory_flags(Katline::MemoryMapFlags flags)
	    -> Arch::Paging::PageFlags;
	auto resolve_target_root(Arch::Process *process)
	    -> Arch::Paging::PageTable *;
	auto rollback_mappings(Arch::Paging::PageTable *root,
	    CL::ArrayList<uptr> const &mapped_virt) -> void;
	auto remember_mapping(Arch::Process *process, uptr address, uptr page_base,
	    usize page_count, usize size, Memory::MemoryObject *object)
	    -> Result<void>;
	auto map_memory_object(Arch::Process *process, Memory::MemoryObject *object,
	    u64 offset, u64 size, Katline::MemoryMapFlags flags,
	    UserPtr<void *> out_addr) -> Result<void>;
	auto unmap_memory_object(Arch::Process *process, uptr address, u64 size)
	    -> Result<void>;

	}
}

namespace Katline::Syscalls {

auto required_page_count(u64 offset, u64 size) -> usize
{
	auto const page_offset { static_cast<usize>(offset % Arch::k_page_size) };
	return static_cast<usize>(
	    (page_offset + size + Arch::k_page_size - 1) / Arch::k_page_size);
}

auto page_flags_from_memory_flags(Katline::MemoryMapFlags flags)
    -> Arch::Paging::PageFlags
{
	if ((flags.raw() & ~k_memory_map_allowed_flags.raw()) != 0)
		return {};

	Arch::Paging::PageFlags page_flags { Arch::Paging::PageFlag::User };
	if (flags.contains(Katline::MemoryMapFlag::Writable))
		page_flags |= Arch::Paging::PageFlag::Writable;
	if (flags.contains(Katline::MemoryMapFlag::NoExecute))
		page_flags |= Arch::Paging::PageFlag::NoExecute;

	return page_flags;
}

auto resolve_target_root(Arch::Process *process) -> Arch::Paging::PageTable *
{
	if (!process)
		return nullptr;

	auto const pt {
		reinterpret_cast<Arch::Paging::PageTable *>(
		    Arch::Paging::phys_to_virt(process->cr3)),
	};
	return pt;
}

auto rollback_mappings(Arch::Paging::PageTable *root,
    CL::ArrayList<uptr> const &mapped_virt) -> void
{
	for (usize i {}; i < mapped_virt.size(); ++i)
		Arch::Paging::unmap(root, *mapped_virt.get(i));
}

auto remember_mapping(Arch::Process *process, uptr address, uptr page_base,
    usize page_count, usize size, Memory::MemoryObject *object) -> Result<void>
{
	if (!process || !object)
		return Result<void>::Err(ErrorsV::InvalidArgument {});

	process->mappings.push(Arch::MemoryMapping {
	    .address = address,
	    .page_base = page_base,
	    .page_count = page_count,
	    .size = size,
	    .object = object,
	});

	return Result<void>::Ok();
}

auto map_memory_object(Arch::Process *process, Memory::MemoryObject *object,
    u64 offset, u64 size, Katline::MemoryMapFlags flags,
    UserPtr<void *> out_addr) -> Result<void>
{
	if (!process || !object || size == 0)
		return Result<void>::Err(ErrorsV::InvalidArgument {});

	if (offset > object->size || size > object->size - offset)
		return Result<void>::Err(ErrorsV::InvalidArgument {});

	auto *root { resolve_target_root(process) };
	if (!root)
		return Result<void>::Err(ErrorsV::InvalidArgument {});

	auto const page_flags { page_flags_from_memory_flags(flags) };
	if (page_flags.raw() == 0)
		return Result<void>::Err(ErrorsV::InvalidArgument {});

	Memory::retain_memory_object(object);

	auto const page_offset { static_cast<usize>(offset % Arch::k_page_size) };
	auto const page_index { static_cast<usize>(offset / Arch::k_page_size) };
	auto const page_count { required_page_count(offset, size) };
	if (page_count == 0) {
		Memory::release_memory_object(object);
		return Result<void>::Err(ErrorsV::InvalidArgument {});
	}

	auto const base { Arch::find_free_user_region(process, page_count) };
	if (!base) {
		Memory::release_memory_object(object);
		return Result<void>::Err(ErrorsV::InvalidArgument {});
	}

	CL::ArrayList<uptr> mapped_virt;
	for (usize i {}; i < page_count; ++i) {
		uptr phys {};
		switch (object->kind) {
		case Memory::MemoryObjectKind::Allocated: {
			auto const phys_page_index { page_index + i };
			auto const phys_opt { object->pages.get(phys_page_index) };
			if (!phys_opt) {
				rollback_mappings(root, mapped_virt);
				Memory::release_memory_object(object);
				return Result<void>::Err(ErrorsV::InvalidArgument {});
			}

			phys = *phys_opt;
			break;
		}
		case Memory::MemoryObjectKind::MMIO:
			phys = object->phys_base
			    + static_cast<uptr>(page_index + i) * Arch::k_page_size;
			break;
		}

		auto const res {
			Arch::Paging::map(
			    root, base + i * Arch::k_page_size, phys, page_flags),
		};
		if (!res) {
			rollback_mappings(root, mapped_virt);
			Memory::release_memory_object(object);
			return Result<void>::Err(ErrorsV::InvalidArgument {});
		}

		mapped_virt.push(base + i * Arch::k_page_size);
	}

	auto const mapped { reinterpret_cast<void *>(base + page_offset) };
	auto const copy_result { copy_out(out_addr, &mapped, sizeof(mapped)) };
	if (copy_result.is_err()) {
		rollback_mappings(root, mapped_virt);
		Memory::release_memory_object(object);
		return copy_result;
	}

	auto const remember_result {
		remember_mapping(process, reinterpret_cast<uptr>(mapped), base,
		    page_count, static_cast<usize>(size), object),
	};
	if (remember_result.is_err()) {
		rollback_mappings(root, mapped_virt);
		Memory::release_memory_object(object);
		return remember_result;
	}

	return Result<void>::Ok();
}

auto unmap_memory_object(Arch::Process *process, uptr address, u64 size)
    -> Result<void>
{
	if (!process || address == 0 || size == 0)
		return Result<void>::Err(ErrorsV::InvalidArgument {});

	for (usize i {}; i < process->mappings.size(); ++i) {
		auto mapping_opt { process->mappings.get(i) };
		if (!mapping_opt)
			continue;
		auto const &mapping { *mapping_opt };

		if (mapping.address != address || mapping.size != size)
			continue;

		auto *root { resolve_target_root(process) };
		if (!root)
			return Result<void>::Err(ErrorsV::InvalidArgument {});

		for (usize j {}; j < mapping.page_count; ++j) {
			if (!Arch::Paging::unmap(
			        root, mapping.page_base + j * Arch::k_page_size))
				return Result<void>::Err(ErrorsV::InvalidArgument {});
		}

		Memory::release_memory_object(mapping.object);
		process->mappings.remove_at(i);
		return Result<void>::Ok();
	}

	return Result<void>::Err(ErrorsV::InvalidArgument {});
}

}
