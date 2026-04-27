#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>
#include <stdint.h>

/* Function Prototypes */
void kheap_init();
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif