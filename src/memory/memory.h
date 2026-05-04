#ifndef MEMORY_H_
#define MEMORY_H_

#include <stddef.h>

void* memset(void* ptr, int c, size_t size);
int memcpy(void* dest, void* src, int bytes);
int memcmp(const void* src, const void* str, int len);

#endif