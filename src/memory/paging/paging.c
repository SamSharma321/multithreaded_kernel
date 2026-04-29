#include <paging.h>
#include "memory/heap/kheap.h"
#include "status.h"

// Current operating directory
static uint32_t* current_directory = 0;

void paging_load_directory(uint32_t* directory);

/* Creating multi-level page table.
    This function emulates VA to PA, but here for simplicity, the mapping is linear.
    There will be 1024 page tables, each containing 1024 pages, all these page tables can be reefrenced
    usingthe page table directory (containing 1024 entries) */
/* Structure of Page Table: 
    PHYSICAL_ADDRESS[31:12] | FLAGS [11:0]
*/
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags) {
    int offset = 0;
    // Paging directories - 1024 directories
    uint32_t* directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
        // Create 1024 page table entries
        uint32_t* entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
        for (int b = 0; b < PAGING_TOTAL_ENTRIES_PER_TABLE; b++) {
            entry[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags;
        }
        // Next set of pages
        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
        directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITABLE;
    }

    struct paging_4gb_chunk* chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;
    return chunk_4gb;
}   

void paging_switch(uint32_t* directory) {
    paging_load_directory(directory);
    current_directory = directory;
}

uint32_t* pagin_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk_4gb) {
    return chunk_4gb->directory_entry;
}

bool paging_is_aligned(void* address) {
    return (((uintptr_t)address % PAGING_PAGE_SIZE) == 0);
}

int paging_get_index(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out) {
    if (!paging_is_aligned(virtual_address)) {
        return -EINVARG;
    }
    // Here VA = PA since it is linearly mapped - so no resolution
    // Resolve the VA into directory entry and table index
    *directory_index_out = (uint32_t)virtual_address / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);

    *table_index_out = (uint32_t)virtual_address % \
        (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE;

    return 0;
}

int paging_set(uint32_t* directory, void* VA, uint32_t PA) {
    uint32_t directory_index = 0;
    uint32_t table_index = 0;

    int status = paging_get_index(VA, &directory_index, &table_index);

    if (status < 0) 
        return status;

    uint32_t entry = directory[directory_index];
    uint32_t* table = (uint32_t*)(entry & 0xfffff000);
    table[table_index] = PA;
    return 0;
}