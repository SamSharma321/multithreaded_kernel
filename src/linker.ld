ENTRY(_start) /* Where the code starts */
OUTPUT_FORMAT("binary") /* not a elf file */
SECTIONS
{
    . = 1M; /* 1 MB in memory - this is the offset in memory at which our kernel sections starts */
    /* That is, the origin will start at 1 MB into the RAM / memory */
    .text :
    {
        *(.text) /* test section */
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(COMMON)
        *(.bss)
    }
}
