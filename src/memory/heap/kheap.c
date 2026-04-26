#include "kheap.h"
#include "heap.h"
#include "config.h"
#include "kernel.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;

void kheap_init() {
    // Allocate 100 MB for this kernel - 1048577600
    // Block size = 4096

    int total_heap_entries = SAMOS_HEAP_SIZE_BYTES / SAMOS_HEAP_BLOCK_SIZE;
    kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY*)SAMOS_HEAP_START_ADDR;
    kernel_heap_table.total = total_heap_entries;


    void* end = (void*)(SAMOS_HEAP_START_ADDR + SAMOS_HEAP_SIZE_BYTES); // Points to end of heap
    int res = heap_create(&kernel_heap, (void*)SAMOS_HEAP_START_ADDR, end, &kernel_heap_table);

    if (res < 0) {
        print("Failed to create heap\n");
        while(1); // panic print
    }
}