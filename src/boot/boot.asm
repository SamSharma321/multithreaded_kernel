; Print "BIOS Execution start..." on screen using a USB stick
; BIOS starts execution from address 0x7c00
; 0x7c0 is the boot sector
ORG 0x7c00               ; Origin to 0
BITS 16                  ; 16 bit architecture

CODE_SEG equ gdt_code - gdt_start       ; Offset of CS
DATA_SEG equ gdt_data - gdt_start       ; Offset of DS, ES, SS, etc

; Setting a dummy BIOS Parameter Block
_start:
    jmp short start
    nop
times 33 db 0            ; 33 bytes of 0's padding for the BPB filling

start:                   ; start label
    jmp 0:step2

step2:
    cli                  ; Clear Interrupts before setting segment registers
    mov ax, 0            ; ORG 0x7c00 makes labels physical addresses already
    mov ds, ax           ; data segment now points at address 0x7c00
    mov es, ax           ; Real mode execution: physical = segment * 16 + offset
    mov ss, ax           ; Stack segment starts from 0x00
    mov sp, 0x7c00       ; Stack pointer starts from the base of stack segment always -> grows downwards.
    sti                  ; Enables Interupts


; ***************** PROTECTED MODE *****************
; https://wiki.osdev.org/Protected_Mode
.load_protected:
    cli                  ; clear all interrupts before changing CPU mode
    lgdt[gdt_descriptor] ; load the GDT with the specified size for the protected mode (CS, DS, etc)
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:load32  ; CODE_SEG is 0x8h
    jmp $


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
    db 0                ; base 16-23 bits
    db 0x9a             ; Access byte
    db 11001111b        ; High 4 bit flags and low 4 bit flags
    db 0                ; Base 24-31 bits

; offset 0x10
gdt_data:               ; DS, SS, ED, FS, GS
    dw 0xffff           ; Segment offset first 0-15 bits
    dw 0                ; base 0-15 bits
    db 0                ; base 16-23 bits
    db 0x92             ; Access byte
    db 11001111b        ; High 4 bit flags and low 4 bit flags
    db 0                ; Base 24-31 bits

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; Size of descriptor table
    dd gdt_start        ; dd - define double word, dw - define word


[BITS 32]               ; Start of 32 bit architecture
load32:
    mov eax, 1          ; staring sector to load - 1, 0 is the boot sector
    mov ecx, 100        ; total sectors to load - completely NULL
    mov edi, 0x0100000  ; address to load the segment into - 1 MB
    call ata_lba_read   ; talk with the drive load sector into memory
    jmp CODE_SEG:0x0100000

; Talking directly to an ATA (IDE) hard drive using I/O ports. 
; Setting up a read operation using LBA (Logical Block Addressing)
ata_lba_read:           ; Reading the ATA harddisk in 32 bit mode
    mov ebx, eax
    mov eax, ebx
    shr eax, 24         ; Right shift by 24 to get MSB 8 bits
    or al, 0xE0         ; Select master drive
    mov dx, 0x1F6
    out dx, al          ; al is 8 bits -> contains the 8 bits of LBA

    ; Send the total sectors to be read from the ATA disk
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; Finished sending the total sectors to read

    ; Send more bits of LBA
    mov eax, ebx
    mov dx, 0x1F3
    out dx, al
    ; Finished

    mov dx, 0x1F4
    mov eax, ebx        ; Restore the backup LBA
    shr eax, 8
    out dx, al
    ; Finished sending more bits of the LBA 

    mov dx, 0x1F5
    mov eax, ebx        ; Restore the backup LBA
    shr eax, 16
    out dx, al

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

; Read all sectors into memory
.next_sector:
    push ecx            ; Store in stack

; Checking if we need to read
.try_again:
    mov dx, 0x1F7
    in al, dx
    test al, 8          ; Check if a bit is set using 8 as bit mask
    jz .try_again

    ; read 256 words at a time
    mov ecx, 256
    mov dx, 0x1F0
    rep insw
    pop ecx
    loop .next_sector
    ; End of reading sectors into memory
    ret




; Boot signature
times 510-($ - $$) db 0 ; Padding with 0's and expand the boot sector into 510 bytes
                        ; Boot sector should be at 511 and 512 bytes
dw 0xAA55               ; 2 byte word - boot signature

