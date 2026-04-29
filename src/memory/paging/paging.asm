[BITS 32]

section .asm
global paging_load_directory
global enable_paging

paging_load_directory:
    push ebp
    mov ebp, esp                ; stacking return address on to stack
    mov eax, [ebp + 8]          ; laoding first argument
    mov cr3, eax                ; contains the physical address of the paging directory base
    pop ebp
    ret

enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000          ; set bit 31 of the control register to enable paging
    mov cr0, eax
    pop ebp
    ret

