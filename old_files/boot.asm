; Print "BIOS Execution start..." on screen using a USB stick
; BIOS starts execution from address 0x7c00
; 0x7c0 is the boot sector
ORG 0                   ; Origin to 0
BITS 16                 ; 16 bit architecture
; Setting a dummy BIOS Parameter Block
_start:
    jmp short start
    nop
times 33 db 0           ; 33 bytes of 0's padding for the BPB filling

start:                  ; start label
    jmp 0x7c0:step2

handle_zero:            ; Example of interrupt
    mov ah, 02h
    mov al, 'A'
    mov bx, 0x00
    int 0x10
    iret                ; interrupt return in assembly language


handle_one:
    mov ah, 02h
    mov al, 'V'
    mov bx, 0x00
    int 0x10
    iret


step2:
    cli                 ; Clear Interrupts before setting segment registers
    mov ax, 0x7c0       ; Can't be fed into the ds or es directly
    mov ds, ax          ; data segment now points at address 0x7c00
    mov es, ax          ; Real mode execution: physical = segment * 16 + offset
    ; Setting the stack segment -> Growing downwards
    mov ax, 0x00
    mov ss, ax          ; Stack segment starts from 0x00
    mov sp, 0x7c00
    sti                 ; Enables Interupts

    ; Interrupt Vector table usually starts at the address 0x0000
    mov word[ss:0x00], handle_zero  ; Use the stack segment - interrupt 0
    mov word[ss:0x02], 0x7c0
    mov word[ss:0x04], handle_one   ; Each entry takes 4 bytes - interrupt 1
    mov word[ss:0x06], 0x7c0

    int 0               ; calls the handle_zero
    int 1               ; calls the handle_one

    mov ax, 0x00
    div ax              ; Same exception is triggered - divide by 0


    ; Routine - Output a message to the terminal/screen
    mov si, message
    call print
    jmp $


; Routine
print:
    mov bx, 0
.loop:
    lodsb               ; the Starting address of string in si -> si is auto-incremented and stored in al
    cmp al, 0           ; String literal character
    je .done            ; jump to .done label if al = 0
    call print_char     ; Call sub-routine
    jmp .loop           ; if sub-routine called, loop again
.done:
    ret                 ; return from sub-routine

; Subroutine
print_char:
    mov ah, 0eh         ; ah belongs to eax register
    int 0x10            ; Interrupt - BIOS routine call for execution
    ret

message: db 'BIOS Execution start...', 0

; Boot signature
times 510-($ - $$) db 0 ; Padding with 0's and expand the boot sector into 510 bytes
                        ; Boot sector should be at 511 and 512 bytes
dw 0xAA55               ; 2 byte word 

; nasm -f bin ./boot.asm -o ./boot.bin -> turn this assembly language to binary
; ndisasm ./boot.bin



