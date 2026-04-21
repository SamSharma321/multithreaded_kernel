ENTRY(_start) /* Where the code starts */
OUTPUT_FORMAT("binary") /* not a elf file */

/* NOTE: when you start mixing C with assembly, it can cause alignment issues */
/* Kernel needs to be the first linked file, so when the device boots, it jumps to the kernel and ot some random c file. */

SECTIONS
{
    . = 1M; /* 1 MB in memory - this is the offset in memory at which our kernel sections starts */
    /* That is, the origin will start at 1 MB into the RAM / memory */
    .text : ALIGN(4096)
    {
        *(.text) /* text section - all code by default will be mapped to this section */
    }

    .rodata : ALIGN(4096)  /* This ALIGN() makes the sections aligned properly and done manually (16 bytes here) */
    {
        *(.rodata)
    }

    .data : ALIGN(4096)
    {
        *(.data)
    }

    .bss : ALIGN(4096)
    {
        *(COMMON)
        *(.bss)
    }

    .asm : ALIGN(4096)
    {
        *(.asm)
    }
    /* Note that if you want a file in certain section of the linker, eg in the asm section, 
     write: section .asm at the top of the file */

}
