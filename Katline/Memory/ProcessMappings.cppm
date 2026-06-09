export module Katline:ProcessMappings;

import CommonLib;
import :ArchConstants;
import :Thread;

export {
	namespace Katline::Arch {

	auto range_overlaps(uptr a_base, uptr a_size, uptr b_base, uptr b_size)
	    -> bool;
	auto find_free_user_region(Process const *process, usize page_count)
	    -> uptr;

	}
}

namespace Katline::Arch {

auto range_overlaps(uptr a_base, uptr a_size, uptr b_base, uptr b_size) -> bool
{
	auto const a_end { a_base + a_size };
	auto const b_end { b_base + b_size };
	if (a_end < a_base || b_end < b_base)
		return true;
	return !(a_end <= b_base || b_end <= a_base);
}

auto find_free_user_region(Process const *process, usize page_count) -> uptr
{
	if (!process || page_count == 0)
		return 0;

	auto const span_bytes { static_cast<uptr>(page_count) * k_page_size };
	auto const search_start { k_page_size };
	if (span_bytes == 0 || search_start > k_user_space_limit - span_bytes)
		return 0;

	for (uptr base { search_start }; base <= k_user_space_limit - span_bytes;
	    base += k_page_size) {
		bool free { true };
		auto const candidate_size { span_bytes };
		for (usize i {}; i < process->mappings.size(); ++i) {
			auto mapping_opt { process->mappings.get(i) };
			if (!mapping_opt)
				continue;

			auto const &mapping { *mapping_opt };
			if (!range_overlaps(base, candidate_size, mapping.address,
			        static_cast<uptr>(mapping.size)))
				continue;

			free = false;
			auto const mapping_end { mapping.address + mapping.size };
			if (mapping_end > base)
				base = mapping_end;
			break;
		}

		if (free)
			return base;
	}

	return 0;
}

}
