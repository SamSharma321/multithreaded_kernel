#ifndef KERNEL_H_
#define KERNEL_H_

/* Predefined Macros */
#define VGA_HEIGHT      25
#define VGA_WIDTH       80
#define SAMOS_MAX_PATH  108

#define ERROR(value)    (void*)(value)
#define ERROR_I(value)  (int)(value)
#define ISERR(value)    ((int)value < 0)

/* Function Prototypes */
void kernel_main();
void print(const char* str);

#endif
