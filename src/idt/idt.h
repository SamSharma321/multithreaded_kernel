#ifndef IDT_H_
#define IDT_H_
// https://wiki.osdev.org/Interrupt_Descriptor_Table
#include <stdint.h>

struct idt_desc {
    uint16_t offset_1;          // Offset bits 0 - 15
    uint16_t selector;          // Selector that in our Global Decriptor Table
    uint8_t zero;               // Should always be zero
    /* Gate Types: Task (0x5), Interrupt (0xE, 0x6), Trap gate (0x7, 0xF) */
    uint8_t type_attr;          // Descriptor type and attributes
    uint16_t offset_2;          // Offset bits 16 - 31
} __attribute__((packed)); /* This makes sure the memory is continuous according to the 64 bit descriptor */

struct idtr_desc {
    uint16_t limit;             // Size of the IDT
    uint32_t base;              // Base address of the IDT
} __attribute__((packed));

/* Function Prototypes */
void idt_init();


#endif