#pragma once

#include <CommonLib/StringView.h>

namespace CL {

[[noreturn]] void panic(StringView const message);

}

namespace std {
struct nothrow_t {
	explicit nothrow_t() = default;
};

struct align_val_t {
	explicit align_val_t(usize a)
	    : value(a)
	{
	}

	usize value;
};

inline constexpr nothrow_t nothrow {};

class bad_alloc { };

}

void *operator new(usize size, void *ptr) noexcept;
void *operator new[](usize size, void *ptr) noexcept;
void operator delete(void *ptr, void *place) noexcept;
void operator delete[](void *ptr, void *place) noexcept;
void *operator new(usize size, std::align_val_t alignment) noexcept;
void *operator new[](usize size, std::align_val_t alignment) noexcept;
void *operator new(
    usize size, std::align_val_t alignment, std::nothrow_t const &) noexcept;
void *operator new(usize size, std::nothrow_t const &) noexcept;
void *operator new[](
    usize size, std::align_val_t alignment, std::nothrow_t const &) noexcept;
void *operator new[](usize size, std::nothrow_t const &) noexcept;
void operator delete(void *ptr) noexcept;
void operator delete[](void *ptr) noexcept;
void operator delete(void *ptr, usize size) noexcept;
void operator delete[](void *ptr, usize size) noexcept;
void operator delete(void *ptr, std::align_val_t) noexcept;
void operator delete[](void *ptr, std::align_val_t) noexcept;
void operator delete(void *ptr, usize size, std::align_val_t) noexcept;
void operator delete[](void *ptr, usize size, std::align_val_t) noexcept;
void operator delete(void *ptr, std::nothrow_t const &) noexcept;
void operator delete[](void *ptr, std::nothrow_t const &) noexcept;
void operator delete(
    void *ptr, std::align_val_t, std::nothrow_t const &) noexcept;
void operator delete[](
    void *ptr, std::align_val_t, std::nothrow_t const &) noexcept;
