#pragma once

using size_t = decltype(sizeof(0));

extern "C" auto memcpy(void *dst, void const *src, size_t size) -> void *;
extern "C" auto memmove(void *dst, void const *src, size_t size) -> void *;
extern "C" auto memset(void *dst, int value, size_t size) -> void *;
extern "C" auto memcmp(void const *lhs, void const *rhs, size_t size) -> int;
extern "C" auto strlen(char const *str) -> size_t;
