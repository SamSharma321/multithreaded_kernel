; Kernel - Protected Mode - 32 bits
[BITS 32]               ; All code under this macro is interpretted as 32 bit code

global _start           ; exports the _start symbol
extern kernel_main      ; when the function is defined elsewhere you use extern, and when you define it in the asm file, you use global
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

    ; Remap the master Programmable Interrupt controller (PIC)
    mov al, 00010001b
    out 0x20, al        ; Start master PIC initialization
    mov al, 0x20        ; Interrupt 0x20 is where master ISR should start
    out 0x21, al
    
    mov al, 0x00000001b
    out 0x21, al
    ; End remap of the master PIC

    call kernel_main    ; Enter C Code B)

    jmp $

; Aligning the kernel image for aligned access - 512 byte alignment
times 512-($ - $$) db 0 