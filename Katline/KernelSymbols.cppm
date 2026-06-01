export module Katline:KernelSymbols;

import CommonLib;

export {
	namespace Katline {

	struct SymbolLookupResult {
		bool found {};
		u64 address {};
		CL::StringView name {};
		CL::StringView file {};
		u32 line {};
	};

	auto lookup_kernel_symbol(u64 address) -> SymbolLookupResult;

	}
}

namespace Katline {

// NOTE: This is kinda ugly, since if you update this struct you need to update
// Util/GenerateKernelSymbols.py as well.
struct KernelSymbol {
	u64 address {};
	char const *name {};
	char const *file {};
	u32 line {};
};

extern "C" {
[[gnu::weak]] KernelSymbol const g_kernel_symbols[] = {};
[[gnu::weak]] usize const g_kernel_symbol_count = 0;
}

auto lookup_kernel_symbol(u64 address) -> SymbolLookupResult
{
	if (g_kernel_symbol_count == 0)
		return {};

	usize lo {};
	usize hi { g_kernel_symbol_count };
	while (lo < hi) {
		auto const mid { lo + ((hi - lo) / 2) };
		auto const mid_addr { g_kernel_symbols[mid].address };
		if (mid_addr <= address)
			lo = mid + 1;
		else
			hi = mid;
	}

	if (lo == 0)
		return {};

	auto const &entry { g_kernel_symbols[lo - 1] };
	if (!entry.name)
		return {};

	return {
		.found = true,
		.address = entry.address,
		.name = CL::StringView { entry.name },
		.file = CL::StringView { entry.file ? entry.file : "" },
		.line = entry.line,
	};
}

}
