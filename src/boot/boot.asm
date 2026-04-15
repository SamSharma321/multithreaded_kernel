; Print "BIOS Execution start..." on screen using a USB stick
; BIOS starts execution from address 0x7c00
; 0x7c0 is the boot sector
ORG 0x7c00              ; Origin to 0
BITS 16                 ; 16 bit architecture

CODE_SEG equ gdt_end - gdt_start        ; Offsets of CS
DATA_SEG equ gdt_data - gdt_start       ; Offset of DS, ES, SS, etc

; Setting a dummy BIOS Parameter Block
_start:
    jmp short start
    nop
times 33 db 0           ; 33 bytes of 0's padding for the BPB filling

start:                  ; start label
    jmp 0:step2

step2:
    cli                 ; Clear Interrupts before setting segment registers
    mov ax, 0x7c0       ; Can't be fed into the ds or es directly
    mov ds, ax          ; data segment now points at address 0x7c00
    mov es, ax          ; Real mode execution: physical = segment * 16 + offset
    mov ss, ax          ; Stack segment starts from 0x00
    mov sp, 0x7c00      ; Stack pointer starts from the base of stack segment always -> grows downwards.
    sti                 ; Enables Interupts

; ***************** PROTECTED MODE *****************
; https://wiki.osdev.org/Protected_Mode
.load_protected:
    cli                  ; clear all interrupts before changing CPU mode
    lgdt[gdt_descriptor] ; load the GDT with the specified size for the protected mode (CS, DS, etc)
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:load32  ; CODE_SEG is 0x8h


; ***************** GLOBAL DESCRIPTOR TABLE ****************
; https://wiki.osdev.org/Global_Descriptor_Table - GDT table decription bitwise
; Global Descriptor table - contains the info on various memory segments
gdt_start:
gdt_null:               ; NULL segment
    dd 0x00
    dd 0x00

; offset 0x08
gdt_code:               ; CS will point to this
    dw 0xffff           ; Segment offset first 0-15 bits
    dw 0                ; base 0-15 bits
    dd 0                ; base 16-23 bits
    db 0x9a             ; Access byte
    db 11001111b        ; High 4 bit flags and low 4 bit flags
    db 0                ; Base 24-31 bits

; offset 0x10
gdt_data:               ; DS, SS, ED, FS, GS
    dw 0xffff           ; Segment offset first 0-15 bits
    dw 0                ; base 0-15 bits
    dd 0                ; base 16-23 bits
    db 0x92             ; Access byte
    db 11001111b        ; High 4 bit flags and low 4 bit flags
    db 0                ; Base 24-31 bits

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; Size of descriptor table
    dd gdt_start        ; dd - define double word, dw - define word

; PROTECTED MODE execution - 32 bit mode
[BITS 32]               ; All code under this macro is interpretted as 32 bit code
load32:
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


; Boot signature
times 510-($ - $$) db 0 ; Padding with 0's and expand the boot sector into 510 bytes
                        ; Boot sector should be at 511 and 512 bytes
dw 0xAA55               ; 2 byte word - boot signature




