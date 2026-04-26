#ifndef CONFIG_H_
#define CONFIG_H_

// Number of total interrupts supported by the system. This is used to allocate the IDT.
#define SAMOS_TOTAL_INTERRUPTS      512
#define KERNEL_CODE_SELECTOR        0x08
#define KERNEL_DATA_SELECTOR        0x10

/* Heap related macros */
#define SAMOS_HEAP_SIZE_BYTES       1048577600  // This would be dynamic based on RAM size in modern OS
#define SAMOS_HEAP_BLOCK_SIZE       4096
#define SAMOS_HEAP_START_ADDR       0x01000000
#define SAMOS_HEAP_TABLE_ADDR       0x00007E00
#endif