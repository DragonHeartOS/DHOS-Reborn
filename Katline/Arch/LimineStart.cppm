module;

#include <Katline/limine.h>

export module Katline:LimineStart;

import CommonLib;
import KatlineAPI;
import :Runtime;

static auto limine_type_to_katline_type(uint64_t type)
    -> Katline::Memory::MemoryType
{
	switch (type) {
	case LIMINE_MEMMAP_USABLE:
		return Katline::Memory::MemoryType::USABLE;
	case LIMINE_MEMMAP_RESERVED:
		return Katline::Memory::MemoryType::RESERVED;
	case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
		return Katline::Memory::MemoryType::ACPI_RECLAIMABLE;
	case LIMINE_MEMMAP_ACPI_NVS:
		return Katline::Memory::MemoryType::ACPI_NVS;
	case LIMINE_MEMMAP_BAD_MEMORY:
		return Katline::Memory::MemoryType::BAD_MEMORY;
	case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
		return Katline::Memory::MemoryType::BOOTLOADER_RECLAIMABLE;
	case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
		return Katline::Memory::MemoryType::KERNEL_AND_MODULES;
	case LIMINE_MEMMAP_FRAMEBUFFER:
		return Katline::Memory::MemoryType::FRAMEBUFFER;
	default:
		return Katline::Memory::MemoryType::RESERVED;
	}
}

static constexpr usize k_max_boot_modules { 256 };
static constexpr usize k_max_boot_framebuffers { 32 };

static constexpr uint64_t kernel_stack_size = 65536;

extern "C" void kernel_start();

[[gnu::used,
    gnu::section(
        ".limine_requests")]] static uint64_t volatile limine_base_revision[]
    = LIMINE_BASE_REVISION(6);

[[gnu::used,
    gnu::section(
        ".limine_"
        "request"
        "s")]] static limine_stack_size_request volatile stack_size_request
    = {
	      .id = LIMINE_STACK_SIZE_REQUEST_ID,
	      .revision = 0,
	      .response = nullptr,
	      .stack_size = kernel_stack_size,
      };

[[gnu::used,
    gnu::section(
        ".limine_"
        "request"
        "s")]] static limine_framebuffer_request volatile framebuffer_request
    = {
	      .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
	      .revision = 0,
	      .response = nullptr,
      };

[[gnu::used,
    gnu::section(
        ".limine_"
        "requests")]] static limine_memmap_request volatile memmap_request
    = {
	      .id = LIMINE_MEMMAP_REQUEST_ID,
	      .revision = 0,
	      .response = nullptr,
      };

[[gnu::used,
    gnu::section(
        ".limine_requests")]] static limine_hhdm_request volatile hhdm_request
    = {
	      .id = LIMINE_HHDM_REQUEST_ID,
	      .revision = 0,
	      .response = nullptr,
      };

[[gnu::used,
    gnu::section(
        ".limine_requests")]] static limine_mp_request volatile mp_request
    = {
	      .id = LIMINE_MP_REQUEST_ID,
	      .revision = 0,
	      .response = nullptr,
	      .flags = LIMINE_MP_REQUEST_X86_64_X2APIC,
      };

[[gnu::used,
    gnu::section(
        ".limine_requests")]] static limine_rsdp_request volatile rsdp_request
    = {
	      .id = LIMINE_RSDP_REQUEST_ID,
	      .revision = 0,
	      .response = nullptr,
      };

[[gnu::used,
    gnu::section(
        ".limine_"
        "request"
        "s")]] static limine_tsc_frequency_request volatile tsc_frequency_request
    = {
	      .id = LIMINE_TSC_FREQUENCY_REQUEST_ID,
	      .revision = 0,
	      .response = nullptr,
      };

[[gnu::used,
    gnu::section(
        ".limine_requests")]] static limine_executable_address_request volatile executable_address_request
    = {
	      .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
	      .revision = 0,
	      .response = nullptr,
      };

[[gnu::used,
    gnu::section(
        ".limine_"
        "request"
        "s")]] static limine_executable_file_request volatile executable_file_request
    = {
	      .id = LIMINE_EXECUTABLE_FILE_REQUEST_ID,
	      .revision = 0,
	      .response = nullptr,
      };

[[gnu::used,
    gnu::section(
        ".limine_"
        "request"
        "s")]] static limine_executable_cmdline_request volatile executable_cmdline_request
    = {
	      .id = LIMINE_EXECUTABLE_CMDLINE_REQUEST_ID,
	      .revision = 0,
	      .response = nullptr,
      };

[[gnu::used,
    gnu::section(".limine_"
                 "request"
                 "s")]] static limine_module_request volatile module_request
    = {
	      .id = LIMINE_MODULE_REQUEST_ID,
	      .revision = 0,
	      .response = nullptr,
	      .internal_module_count = 0,
	      .internal_modules = nullptr,
      };

[[gnu::used,
    gnu::section(
        ".limine_requests_"
        "start")]] static uint64_t volatile limine_requests_start_marker[]
    = LIMINE_REQUESTS_START_MARKER;
[[gnu::used,
    gnu::section(".limine_requests_"
                 "end")]] static uint64_t volatile limine_requests_end_marker[]
    = LIMINE_REQUESTS_END_MARKER;

extern "C" auto kernel_start() -> void
{
	if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) {
		for (;;)
			asm("hlt");
	}

	if (framebuffer_request.response == nullptr
	    || framebuffer_request.response->framebuffer_count == 0
	    || memmap_request.response == nullptr
	    || hhdm_request.response == nullptr || rsdp_request.response == nullptr
	    || mp_request.response == nullptr
	    || tsc_frequency_request.response == nullptr
	    || executable_cmdline_request.response == nullptr
	    || executable_address_request.response == nullptr
	    || executable_file_request.response == nullptr
	    || executable_file_request.response->executable_file == nullptr) {
		for (;;)
			asm("hlt");
	}

	auto *const fb_tag { framebuffer_request.response->framebuffers[0] };
	auto *const mmap_tag { memmap_request.response };
	auto const hhdm_offset { hhdm_request.response->offset };

	if (fb_tag->bpp != 32) {
		for (;;)
			asm("hlt");
	}

	Katline::Controller::Framebuffer fb {
		(u64)fb_tag->address,
		(u16)fb_tag->width,
		(u16)fb_tag->height,
		(u16)fb_tag->pitch,
		fb_tag->bpp,
		(u8 *)fb_tag->address,
	};

	static Katline::Memory::MemoryData memory_map_entries[256];
	auto memory_map_entry_count { mmap_tag->entry_count };
	if (memory_map_entry_count > 256)
		memory_map_entry_count = 256;

	for (u64 i {}; i < memory_map_entry_count; ++i) {
		auto *const entry { mmap_tag->entries[i] };
		auto base { entry->base };

		if (limine_type_to_katline_type(entry->type)
		    == Katline::Memory::MemoryType::USABLE)
			base += hhdm_offset;

		memory_map_entries[i] = {
			.base = base,
			.size = entry->length,
			.type = limine_type_to_katline_type(entry->type),
			.unused = 0,
		};
	}

	Katline::Memory::MemoryMap mmap = {
		.size = memory_map_entry_count,
		.data = memory_map_entries,
	};

	static Katline::BootModule boot_modules[k_max_boot_modules];
	usize boot_module_count {};
	if (module_request.response && module_request.response->modules) {
		boot_module_count = module_request.response->module_count;
		if (boot_module_count > k_max_boot_modules)
			boot_module_count = k_max_boot_modules;

		for (usize i {}; i < boot_module_count; ++i) {
			auto *const module { module_request.response->modules[i] };
			auto const *const name {
				module && module->string     ? module->string
				    : module && module->path ? module->path
				                             : nullptr,
			};
			boot_modules[i] = Katline::BootModule {
				.name = CL::Span<char const>(name, strlen(name)),
				.address = module ? module->address : nullptr,
				.size = module ? module->size : 0,
			};
		}
	}

	static Katline::BootFramebuffer boot_framebuffers[k_max_boot_framebuffers];
	usize boot_framebuffer_count {};
	if (framebuffer_request.response
	    && framebuffer_request.response->framebuffers) {
		boot_framebuffer_count
		    = framebuffer_request.response->framebuffer_count;
		if (boot_framebuffer_count > k_max_boot_framebuffers)
			boot_framebuffer_count = k_max_boot_framebuffers;

		for (usize i {}; i < boot_framebuffer_count; ++i) {
			auto const *const framebuffer {
				framebuffer_request.response->framebuffers[i]
			};
			boot_framebuffers[i] = Katline::BootFramebuffer {
				.address = framebuffer ? framebuffer->address : nullptr,
				.width = framebuffer ? framebuffer->width : 0,
				.height = framebuffer ? framebuffer->height : 0,
				.pitch = framebuffer ? framebuffer->pitch : 0,
				.bpp = framebuffer ? static_cast<u64>(framebuffer->bpp) : 0,
				.format
				= framebuffer ? static_cast<u64>(framebuffer->memory_model) : 0,
				.edid = CL::Span<u8 const>(
				    reinterpret_cast<u8 const *>(
				        framebuffer ? framebuffer->edid : nullptr),
				    framebuffer ? framebuffer->edid_size : 0),
			};
		}
	}

	Katline::BootData const boot_data {
		.cmdline = {
			.text = CL::Span<char const>(executable_cmdline_request.response->cmdline,
			    strlen(executable_cmdline_request.response->cmdline)),
		},
		.modules = CL::Span<Katline::BootModule const>(boot_modules,
		    boot_module_count),
		.framebuffers = CL::Span<Katline::BootFramebuffer const>(
		    boot_framebuffers, boot_framebuffer_count),
	};

	for (uint i {}; i < mp_request.response->cpu_count; i++) {
		auto *cpu = mp_request.response->cpus[i];
		cpu->goto_address = [](limine_mp_info *info) {
			Katline::boot_cpu(info->lapic_id, info->processor_id,
			    info->extra_argument,
			    tsc_frequency_request.response->frequency);
		};
	}

	Katline::StartupInfo info {
		.framebuffer = &fb,
		.mmap = &mmap,
		.cpu_count = static_cast<u32>(mp_request.response->cpu_count),
		.bsp_lapic_id = mp_request.response->bsp_lapic_id,
		.rsdp_address = reinterpret_cast<uptr>(rsdp_request.response->address),
		.hhdm_offset = hhdm_offset,
		.tsc_frequency_hz = tsc_frequency_request.response->frequency,
		.stack_size = kernel_stack_size,
		.kernel_physical_base
		= executable_address_request.response->physical_base,
		.kernel_size = executable_file_request.response->executable_file->size,
	};

	Katline::katline_main(info, boot_data);

	for (;;)
		asm("hlt");
}
