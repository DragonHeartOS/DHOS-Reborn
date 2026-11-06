#include <new>

import CommonLib;
import KatlineAPI;

namespace {

struct AllocationHeader {
	Katline::Handle memory_object {};
	void *mapping {};
	unsigned long mapping_size {};
	u64 magic {};
};

static constexpr u64 k_allocation_magic { 0x44484f532e616c6cull };
static constexpr unsigned long k_default_alignment { 16 };

static auto allocation_alignment(unsigned long alignment) -> unsigned long
{
	if (alignment < alignof(AllocationHeader))
		alignment = alignof(AllocationHeader);
	if ((alignment & (alignment - 1)) != 0)
		alignment = alignof(AllocationHeader);
	return alignment;
}

static auto allocate_block(unsigned long size, unsigned long alignment)
    -> void *
{
	if (size == 0)
		size = 1;

	alignment = allocation_alignment(alignment);

	auto const total_size {
		CL::Memory::align_up(size + alignment + sizeof(AllocationHeader),
		    alignof(unsigned long)),
	};

	auto const memory_object_res
	    = Katline::Syscalls::sys_memory_create(total_size);
	if (memory_object_res.is_err())
		return nullptr;

	auto const memory_object { memory_object_res.get_ok_unsafe() };
	void *mapping {};
	auto const map_res = Katline::Syscalls::sys_memory_map(memory_object, 0,
	    total_size,
	    Katline::MemoryMapFlags { Katline::MemoryMapFlag::Writable }, &mapping);
	if (map_res.is_err()) {
		Katline::Syscalls::sys_handle_close(memory_object);
		return nullptr;
	}

	auto const raw { reinterpret_cast<unsigned long>(mapping) };
	auto const user { CL::Memory::align_up(
		raw + sizeof(AllocationHeader), alignment) };
	auto *header { reinterpret_cast<AllocationHeader *>(
		user - sizeof(AllocationHeader)) };
	header->memory_object = memory_object;
	header->mapping = mapping;
	header->mapping_size = total_size;
	header->magic = k_allocation_magic;
	return reinterpret_cast<void *>(user);
}

static auto release_block(void *ptr) -> void
{
	if (!ptr)
		return;

	auto *header { reinterpret_cast<AllocationHeader *>(
		reinterpret_cast<unsigned long>(ptr) - sizeof(AllocationHeader)) };
	if (header->magic != k_allocation_magic)
		return;

	Katline::Syscalls::sys_memory_unmap(header->mapping, header->mapping_size);
	Katline::Syscalls::sys_handle_close(header->memory_object);
	header->magic = 0;
}

} // namespace

void *operator new(unsigned long size)
{
	return allocate_block(size, k_default_alignment);
}

void *operator new[](unsigned long size)
{
	return allocate_block(size, k_default_alignment);
}

void *operator new(unsigned long size, std::nothrow_t const &) noexcept
{
	return allocate_block(size, k_default_alignment);
}

void *operator new[](unsigned long size, std::nothrow_t const &) noexcept
{
	return allocate_block(size, k_default_alignment);
}

void *operator new(unsigned long size, std::align_val_t alignment)
{
	return allocate_block(size, static_cast<unsigned long>(alignment));
}

void *operator new[](unsigned long size, std::align_val_t alignment)
{
	return allocate_block(size, static_cast<unsigned long>(alignment));
}

void *operator new(unsigned long size, std::align_val_t alignment,
    std::nothrow_t const &) noexcept
{
	return allocate_block(size, static_cast<unsigned long>(alignment));
}

void *operator new[](unsigned long size, std::align_val_t alignment,
    std::nothrow_t const &) noexcept
{
	return allocate_block(size, static_cast<unsigned long>(alignment));
}

void operator delete(void *ptr) noexcept { release_block(ptr); }

void operator delete[](void *ptr) noexcept { release_block(ptr); }

void operator delete(void *ptr, std::nothrow_t const &) noexcept
{
	release_block(ptr);
}

void operator delete[](void *ptr, std::nothrow_t const &) noexcept
{
	release_block(ptr);
}

void operator delete(void *ptr, std::align_val_t) noexcept
{
	release_block(ptr);
}

void operator delete[](void *ptr, std::align_val_t) noexcept
{
	release_block(ptr);
}

void operator delete(
    void *ptr, std::align_val_t, std::nothrow_t const &) noexcept
{
	release_block(ptr);
}

void operator delete[](
    void *ptr, std::align_val_t, std::nothrow_t const &) noexcept
{
	release_block(ptr);
}

void operator delete(void *ptr, unsigned long) noexcept { release_block(ptr); }

void operator delete[](void *ptr, unsigned long) noexcept
{
	release_block(ptr);
}

void operator delete(void *ptr, unsigned long, std::align_val_t) noexcept
{
	release_block(ptr);
}

void operator delete[](void *ptr, unsigned long, std::align_val_t) noexcept
{
	release_block(ptr);
}
