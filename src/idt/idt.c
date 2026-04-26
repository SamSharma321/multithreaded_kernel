#include "idt.h"
#include "config.h"
#include "memory/memory.h"
#include "kernel.h"
#include "io/io.h"

struct idt_desc idt_descriptors[SAMOS_TOTAL_INTERRUPTS];    // 512 interrupt descriptions
struct idtr_desc idtr_descriptor;                           // Only one register

extern void int21h();
extern void no_interrupt();

void int21h_handler() {
    print("Keyboard Pressed");
    outb(0x20, 0x20);       // Acknowledge the PIC ISR
}

void no_interrupt_handler() {
    outb(0x20, 0x20);
}

void idt_set(int int_num, void* address) {
    idt_descriptors[int_num].offset_1 = (uint32_t)address & 0x0000FFFF;
    idt_descriptors[int_num].offset_2 = (uint32_t)address >> 16u;
    idt_descriptors[int_num].selector = KERNEL_CODE_SELECTOR;
    idt_descriptors[int_num].zero = 0x00u;
    // Previledge level = ring 3 = 3 -> user access to interrupts available
    // Present = 1
    // Storage_Seg = 0
    // Gate type = 0xE -> Interrupt Gate
    idt_descriptors[int_num].type_attr = 0xEEu; /* PRESENT[7] | PREVILEDGE_LEVEL[6:5] | STORAGE_SEG[4] | GATE_TYPE[3:0] */
}

extern void idt_load(void* ptr);

void idt_zero() { // Deivide by zero error
    print("Divide by zero error.\n");  
    while (1) {}
}

void idt_init() {
    memset(idt_descriptors, 0, sizeof(idt_descriptors)); // Null descriptors

    // Initialize the IDTR register
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uint32_t)&idt_descriptors[0];

    // Divide by zero interrupt
    idt_set(0, idt_zero);
    // idt_set(0x20, int21h); Timer INterrupt is 0x20
    idt_set(0x21, int21h);


    // Set handlers for other interrupts
    // The interrupts can be called from kernel.asm by simulating them
    // int xx where xx is the interrupt entry

    // Load the interrupt table
    idt_load(& idtr_descriptor);
}



