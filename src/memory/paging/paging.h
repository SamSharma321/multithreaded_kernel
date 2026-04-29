#ifndef PAGING_H_
#define PAGING_H_

#include <stddef.h>
#include <stdint.h>

#define PAGING_CACHE_DISABLED   0b00010000      // Bit mask to disable page caching
#define PAGING_WRITE_THROUGH    0b00001000
#define PAGING_ACCESS_FROM_ALL  0b00000100      // All rings can access the current page
#define PAGING_IS_WRITABLE      0b00000010
#define PAGING_IS_PRESENT       0b00000001


#define PAGING_TOTAL_ENTRIES_PER_TABLE  1024u
#define PAGING_PAGE_SIZE                4096u


struct paging_4gb_chunk {
    uint32_t* directory_entry;   // Pointer to page directory
};

/* function Prototypes */
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags);
void paging_switch(uint32_t* directory);
void enable_paging();


#endif