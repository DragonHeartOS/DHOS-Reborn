export module KatlineAPI:Types;

import CommonLib;

export {
	namespace Katline {

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

	}
}
