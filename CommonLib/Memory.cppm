export module CommonLib:Memory;

import :TypeTraits;
import :Types;

export {
	namespace CL::Memory {

	template<typename T>
	requires(CL::IsIntegralV<CL::RemoveConstRef<T>>
	    && __is_unsigned(CL::RemoveConstRef<T>))
	constexpr auto align_down(T value, T alignment) -> T
	{
		if (alignment == 0)
			return value;

		return static_cast<T>(value & static_cast<T>(~(alignment - 1)));
	}

	template<typename T>
	requires(CL::IsIntegralV<CL::RemoveConstRef<T>>
	    && __is_unsigned(CL::RemoveConstRef<T>))
	constexpr auto align_up(T value, T alignment) -> T
	{
		if (alignment == 0)
			return value;

		T const mask { static_cast<T>(alignment - 1) };
		T const add { static_cast<T>(value + mask) };
		if (add < value)
			return static_cast<T>(~static_cast<T>(0));

		return static_cast<T>(add & static_cast<T>(~mask));
	}

	template<typename T>
	requires(CL::IsIntegralV<CL::RemoveConstRef<T>>
	    && __is_unsigned(CL::RemoveConstRef<T>))
	constexpr auto ranges_overlap(T a_base, T a_size, T b_base, T b_size)
	    -> bool
	{
		if (a_size == 0 || b_size == 0)
			return false;

		auto a_end { static_cast<CL::RemoveConstRef<T>>(a_base + a_size) };
		if (a_end < a_base)
			a_end = static_cast<CL::RemoveConstRef<T>>(
			    ~static_cast<CL::RemoveConstRef<T>>(0));

		auto b_end { static_cast<CL::RemoveConstRef<T>>(b_base + b_size) };
		if (b_end < b_base)
			b_end = static_cast<CL::RemoveConstRef<T>>(
			    ~static_cast<CL::RemoveConstRef<T>>(0));

		return a_base < b_end && b_base < a_end;
	}

	template<typename T>
	requires(CL::IsIntegralV<CL::RemoveConstRef<T>>
	    && __is_unsigned(CL::RemoveConstRef<T>))
	constexpr auto range_contains(
	    T outer_base, T outer_size, T inner_base, T inner_size) -> bool
	{
		if (inner_size == 0)
			return true;

		T outer_end { static_cast<T>(outer_base + outer_size) };
		if (outer_end < outer_base)
			outer_end = static_cast<T>(~static_cast<T>(0));

		T inner_end { static_cast<T>(inner_base + inner_size) };
		if (inner_end < inner_base)
			inner_end = static_cast<T>(~static_cast<T>(0));

		return inner_base >= outer_base && inner_end <= outer_end;
	}

	}
}
