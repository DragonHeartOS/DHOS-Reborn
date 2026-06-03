#ifndef LIBC_STRING_H
#define LIBC_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

typedef __SIZE_TYPE__ size_t;

size_t strlen(char const *str);
void *memcpy(void *dst, void const *src, size_t size);
void *memmove(void *dst, void const *src, size_t size);
void *memset(void *dst, int value, size_t size);

#ifdef __cplusplus
}
#endif

#endif
