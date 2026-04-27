#ifndef HEAP_H_
#define HEAP_H_
#include "config.h"
#include <stdint.h>
#include <stddef.h>


#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 0x01
#define HEAP_BLOCK_TABLE_ENTRY_FREE 0x00
#define HEAP_BLOCK_HAS_NEXT 0b10000000
#define HEAP_BLOCK_IS_FIRST 0b01000000

typedef unsigned char HEAP_BLOCK_TABLE_ENTRY;

struct heap_table {
    HEAP_BLOCK_TABLE_ENTRY* entries;  // Not making an array in the kernel - do it in untime
    size_t total;
};

struct heap {
    struct heap_table* h_table;
    void* addr;             // start address
};

/* Function Prototypes */
int heap_create(struct heap* heap, void* start_ptr, void* end, struct heap_table* h_table);
void* heap_malloc(struct heap* heap, size_t size);
void heap_free(struct heap* heap, void* ptr);

#endif