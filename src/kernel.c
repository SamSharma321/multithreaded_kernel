#include "kernel.h"
#include "stdint.h"
#include <stddef.h>
#include <idt/idt.h>
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"


/* Predefined Macros */
#define VIDEO_MEM_ADDRESS ((volatile uint16_t*)0xB8000)
#define BLUE_COLOR 1u
#define GREEN_COLOR 2u
#define BLACK_COLOR 0u
#define WHITE_COLOR 15u

/* Function Macros */
/*! Macro to display a character in a particular color */
#define CFG_CHAR_VIDEO(color, data) ((color << 0x08u)| (uint16_t)(data))
#define TERMINAL_DISPLAY(i, j) video_mem[(i * VGA_WIDTH) + j]

volatile uint16_t* video_mem = 0;
uint16_t terminal_row = 0u;
uint16_t terminal_col = 0u;

void terminal_initialize() {
    terminal_row = 0;
    terminal_col = 0;
    video_mem = VIDEO_MEM_ADDRESS;
    for (uint16_t i = 0; i < VGA_HEIGHT; i++) {
        for (uint16_t j = 0; j < VGA_WIDTH; j++) {
            TERMINAL_DISPLAY(i, j) = CFG_CHAR_VIDEO(BLACK_COLOR, ' ');
        }
    }
    return;
}

void terminal_putchar(int x, int y, char c, char color) {
    TERMINAL_DISPLAY(y, x) = CFG_CHAR_VIDEO(color, c);
    return;
}

size_t strlen(char* str) {
    size_t len = 0;
    while(str[len]) {
        len++;
    }
    return len;
}

void terminal_writechar(char c, char color) {
    if (c == '\n') {
        // Move to next line
        terminal_row++;
        terminal_col = 0;
    } else if (c == '\t') {
        terminal_col += 4;
    } else {
        terminal_putchar(terminal_col++, terminal_row, c, color);
    }
    if (terminal_col >= VGA_WIDTH) {
        terminal_col = 0;
        terminal_row += 1;
    }
}

/*! @brief: Function to print a string to the display. */
void print(const char* str) {
    terminal_writechar(*str, WHITE_COLOR);
    while(*(str++)) {
        terminal_writechar(*str, WHITE_COLOR);
    }
}

static struct paging_4gb_chunk* kernel_chunk = 0;

void kernel_main() {
    terminal_initialize();
    print("SAMOS: System booted successfully!\n");
    /********** KERNEL HEAP ***********/
    kheap_init();

    /********** INTERRUPT SECTION **********/
    // Initializing IDT
    idt_init();

    /******* PAGING SECTION  */
    // Flags -> Write back + 4 Kb page + Readable + Supervisor only + Cacheable
    kernel_chunk = paging_new_4gb(PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT | PAGING_IS_WRITABLE);
    // switch to kernel paging chunk
    paging_switch(pagin_4gb_chunk_get_directory(kernel_chunk));
    // enable paging
    enable_paging();

    /* Example for accessing and writing data using virtual address */
    char* ptr = kzalloc(4096); // This is the physical address
    // 4096 byte aligned - lower bits will be 0's
    uint32_t VA_start = 0x1000;
    paging_set(pagin_4gb_chunk_get_directory(kernel_chunk), (void*)VA_start, (uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT | PAGING_IS_WRITABLE);
    char* ptr2 = (char*)VA_start; // VA set above
    char* str[] = "Virtual Address enabled\n";
    memcpy(ptr2, str, 24);  // copying to virtual address
    print(ptr2);

    // enable interrupts
    enable_interrupts();
    
    return;
}
