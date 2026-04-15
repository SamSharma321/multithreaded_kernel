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

step2:
    cli                 ; Clear Interrupts before setting segment registers
    mov ax, 0x7c0       ; Can't be fed into the ds or es directly
    mov ds, ax          ; data segment now points at address 0x7c00
    mov es, ax          ; Real mode execution: physical = segment * 16 + offset
    ; Setting the stack segment -> Growing downwards
    mov ax, 0x00
    mov ss, ax          ; Stack segment starts from 0x00
    mov sp, 0x7c00      ; Stack pointer starts from the base of stack segment always -> grows downwards.
    sti                 ; Enables Interupts

    ; ********************* LOAD DATA FROM HARD DISK ***********************
    ; http://www.ctyme.com/intr/rb-0607.htm - assuming cyclinder-head hard-disk
    ; Step 1. Initialize what to read
    mov ah, 2           ; Read sector command
    mov al, 1           ; Only read one sector of memory
    mov ch, 0           ; cylinder command = 0
    mov cl, 2           ; read sector 2
    mov dh, 0           ; head number
    mov bx, buffer
    ; Step 2. Trigger interrupt
    int 0x13            ; BIOS DISK SERVICE INTERRUPT - read sector from the disk
    jc error_harddisk   ; jump carry - if the carry flag is set, then the disk sector read failed
    mov si, buffer
    call print
    jmp $               ; infinite jump

; ************************* ROUTINES **************************
; Error message if data sector from harddisk is failed to be read
error_harddisk:
    mov si, error_message
    call print
    jmp $               ; Infinite jump

; Routine for printing a message
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

error_message: db "Failed to load sector", 0

; Boot signature
times 510-($ - $$) db 0 ; Padding with 0's and expand the boot sector into 510 bytes
                        ; Boot sector should be at 511 and 512 bytes
dw 0xAA55               ; 2 byte word - boot signature


buffer:                 ; Will automoatically point to the segment containing the data - 0x7E00 address





