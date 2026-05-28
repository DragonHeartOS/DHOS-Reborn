#pragma once

namespace CL {

namespace detail::adl {

void to_display_string();
void to_debug_string();

}

template<typename T> T &&move(T &value) { return static_cast<T &&>(value); }

template<typename T> constexpr T &&forward(T &value) noexcept
{
	return static_cast<T &&>(value);
}

template<typename T> constexpr T &&forward(T &&value) noexcept
{
	return static_cast<T &&>(value);
}

template<typename T> constexpr void swap(T &a, T &b)
{
	T tmp = CL::move(a);
	a = CL::move(b);
	b = CL::move(tmp);
}

template<typename T> struct InPlace { };

inline constexpr struct ToDisplayStringFn {
	template<typename T> constexpr auto operator()(T &&value) const -> decltype(auto)
	{
		using detail::adl::to_display_string;
		return to_display_string(forward<T>(value));
	}
} to_display_string {};

inline constexpr struct ToDebugStringFn {
	template<typename T> constexpr auto operator()(T &&value) const -> decltype(auto)
	{
		using detail::adl::to_debug_string;
		return to_debug_string(forward<T>(value));
	}
} to_debug_string {};

}

#define UNREACHABLE() __builtin_unreachable()
