#ifndef CONFIG_H_
#define CONFIG_H_

// Number of total interrupts supported by the system. This is used to allocate the IDT.
#define SAMOS_TOTAL_INTERRUPTS      512
#define KERNEL_CODE_SELECTOR        0x08
#define KERNEL_DATA_SELECTOR        0x10

#endif