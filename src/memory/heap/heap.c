#include "heap.h"
#include "kernel.h"
#include "stdbool.h"
#include "status.h"
#include "memory/memory.h"

static bool validate_alignment(void* ptr) {
    return ((unsigned int)ptr % SAMOS_HEAP_BLOCK_SIZE == 0);
}

static int heap_validate_table(void* start, void* end, struct heap_table* h_table) {
    size_t table_size = (size_t)(end - start);
    size_t total_blocks = table_size / SAMOS_HEAP_BLOCK_SIZE;

    if (total_blocks != h_table->total)
        return -EINVARG;

    

    return SAMOS_ALL_OK;
}

int heap_create(struct heap* heap, void* start_ptr, void* end, struct heap_table* h_table) {
    if (validate_alignment(start_ptr) || validate_alignment(end)) {
        return -EINVARG;
    }

    if ((heap == NULL) || (start_ptr == NULL) || (h_table == NULL)) {
        return -EIO;
    }
    if ((HEAP_BLOCK_TABLE_ENTRY*)end < (HEAP_BLOCK_TABLE_ENTRY*)start_ptr) {
        return -EIO;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->addr = start_ptr;
    heap->h_table = h_table;

    int res = heap_validate_table(start_ptr, end, h_table);
    if (res != SAMOS_ALL_OK) return res;

    // initialize all table entries to 0
    size_t heap_table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * h_table->total;
    memset(h_table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, heap_table_size);

    return SAMOS_ALL_OK;
}

static size_t heap_align_value_to_upper(size_t val) {
    if (val % SAMOS_HEAP_BLOCK_SIZE == 0) {
        return val;
    }
    return (val - (val % SAMOS_HEAP_BLOCK_SIZE) + SAMOS_HEAP_BLOCK_SIZE);
}

static int heap_get_start_block(struct heap* heap, uint32_t total_blocks) {
    struct heap_table* table = heap->h_table;
    int bc = 0;
    int bs = -1;

    for (int i = 0; i < table->total; i++) {
        if ((table->entries[i] & 0x0Fu) != HEAP_BLOCK_TABLE_ENTRY_FREE) {
            bc = 0;
            bs = -1;
            continue;
        }

        if (bs == -1) {
            bs = i;
        }
        bc++;

        if (bc = total_blocks) {
            break;
        }
    }

    return bs;
}

void* heap_to_block_address(struct heap* heap, uint32_t start_block) {
    return (HEAP_BLOCK_TABLE_ENTRY)(heap->addr) + (start_block * SAMOS_HEAP_BLOCK_SIZE);
}

void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks) {
    void* address = 0;
    int start_block = heap_get_start_block(heap, total_blocks);
    if (start_block < 0)
        return address;
    address = heap_block_to_address(heap, start_block);
    // Mark the block as marked
    heap_mark_blocks_taken(heap, start_block, total_blocks);
    return address;
}

void* heap_malloc(struct heap* heap, size_t size) {
    size_t aligned_size = heap_align_value_to_upper(size);
    uint32_t total_blocks = aligned_size / SAMOS_HEAP_BLOCK_SIZE;

    return heap_malloc_blocks(heap, total_blocks);
}


void heap_free(struct heap* heap) {
    return;
}