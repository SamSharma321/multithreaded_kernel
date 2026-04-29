#include <paging.h>
#include "memory/heap/kheap.h"

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

static uint32_t* pagin_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk_4gb) {
    return chunk_4gb->directory_entry;
}

