#pragma once

#include <CommonLib/TypeTraits.h>
#include <CommonLib/Types.h>

namespace CL {

template<typename CharTypeT> struct BaseString;
template<typename CharTypeT> struct BaseStringView;

namespace detail::adl {

auto to_display_string(BaseString<char> const &) -> BaseString<char>;
auto to_display_string(BaseStringView<char> const) -> BaseString<char>;
auto to_display_string(char const *) -> BaseString<char>;

auto to_debug_string(BaseString<char> const &) -> BaseString<char>;
auto to_debug_string(BaseStringView<char> const) -> BaseString<char>;
auto to_debug_string(char const *) -> BaseString<char>;

auto to_hash(BaseString<char> const &) -> usize;
auto to_hash(BaseStringView<char> const) -> usize;
auto to_hash(char const *) -> usize;

template<typename T>
requires(IsIntegralV<RemoveConstRef<T>>)
constexpr auto to_hash(T value) -> usize
{
	return static_cast<usize>(value);
}

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
	template<typename T>
	constexpr auto operator()(T &&value) const -> decltype(auto)
	{
		using detail::adl::to_display_string;
		return to_display_string(forward<T>(value));
	}
} to_display_string {};

inline constexpr struct ToDebugStringFn {
	template<typename T>
	constexpr auto operator()(T &&value) const -> decltype(auto)
	{
		using detail::adl::to_debug_string;
		return to_debug_string(forward<T>(value));
	}
} to_debug_string {};

inline constexpr struct ToHashFn {
	template<typename T>
	constexpr auto operator()(T &&value) const -> decltype(auto)
	{
		using detail::adl::to_hash;
		return to_hash(forward<T>(value));
	}
} to_hash {};

template<typename... Ts> [[gnu::always_inline]] void ignore_unused(Ts &&...) { }

template<typename... Ts>
[[gnu::always_inline]] void ignore_unused(Ts const &...)
{
}

template<typename... Ts> [[gnu::always_inline]] void ignore_unused() { }

}

#define UNREACHABLE() __builtin_unreachable()
