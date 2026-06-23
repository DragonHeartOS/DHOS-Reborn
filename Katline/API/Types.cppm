export module KatlineAPI:Types;

import CommonLib;

export {
	namespace Katline {

	struct BootCmdlineView {
		CL::Span<char const> text;
	};

	struct BootModule {
		CL::Span<char const> name;
		void const *address;
		u64 size;
	};

	struct BootFramebuffer {
		void *address;
		u64 width;
		u64 height;
		u64 pitch;
		u64 bpp;
		u64 format;
		CL::Span<u8 const> edid;
	};

	struct BootData {
		BootCmdlineView cmdline;
		CL::Span<BootModule const> modules;
		CL::Span<BootFramebuffer const> framebuffers;
	};

	enum class ProcessCapability : u64 {
		ManageCaps = 1ull << 0,
		BootInfo = 1ull << 1,
		BootFramebuffer = 1ull << 2,
		MapMMIO = 1ull << 3,
		ManageProcesses = 1ull << 4,
		BootModules = 1ull << 5,
		BootCmdline = 1ull << 6,
		IOPort = 1ull << 7,
	};

	using ProcessCapabilityFlags = CL::Flags<ProcessCapability>;

	struct Handle {
		u64 id {};

		constexpr auto is_invalid() const -> bool { return id == 0; }
	};

	struct ProcessID {
		u64 id;
	};

	struct ThreadID {
		u64 id;
	};

	struct ProcessInfo {
		ProcessID pid;
		u64 handle_count;
		u64 name_len;
		char const name[];
	};

	enum class MemoryMapFlag : u64 {
		Writable = 1ull << 0,
		NoExecute = 1ull << 1,
	};

	using MemoryMapFlags = CL::Flags<MemoryMapFlag>;

	struct ProcessMemoryMapOptions {
		Handle process;
		Handle memory_object;
		u64 offset;
		u64 size;
		MemoryMapFlags flags;
		void *addr;
	};

	struct BootInfoTable {
		u64 total_len;
		u64 modules_offset;
		u64 modules_len;
		u64 cmdline_offset;
		u64 cmdline_len;
		u64 framebuffers_offset;
		u64 framebuffers_len;
	};

	struct BootCmdline {
		u64 len;
		char data[];
	};

	struct BootModuleList {
		u64 len;
		u64 entries_offset;
	};

	struct BootModuleEntry {
		u64 id;
		u64 name_offset;
		u64 name_len;
		u64 size;
		u64 flags;
	};

	struct BootFramebufferList {
		u64 len;
		u64 entries_offset;
	};

	struct BootFramebufferEntry {
		u64 id;
		u64 width;
		u64 height;
		u64 pitch;
		u64 bpp;
		u64 format;
		u64 size;
		u64 edid_len;
		u64 edid_offset;
		u64 flags;
	};

	enum class KernelMessageType : u64 {
		GetBootInfo = 1,
		GetBootModuleMemoryObjectById = 2,
		GetCmdlineMemoryObject = 3,
		GetFramebufferMemoryObjectById = 4,
		GrantIOPortRange = 5,
		GetPageSize = 6,
	};

	}
}
