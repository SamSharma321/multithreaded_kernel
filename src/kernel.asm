; Kernel - Protected Mode - 32 bits
[BITS 32]               ; All code under this macro is interpretted as 32 bit code
global _start           ; exports the _start symbol
CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    mov ax, DATA_SEG
    mov ds, ax          ; Initializing DS, SS< FS, ED, GS
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000  ; Set the base pointer to point to this location
    mov esp, ebp        ; set stack pointer to base pointer

    ; enable A20 - accessing 21st bit of every memory access
    in al, 0x92
    or al, 2
    out 0x92, al

    jmp $