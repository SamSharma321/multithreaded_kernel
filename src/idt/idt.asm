section .asm

global idt_load
global int21h
global no_interrupt
global enable_interrupts
global disable_interrupts

extern int21h_handler
extern no_interrupt_handler

enable_interrupts:
    sti
    ret

disable_interrupts:
    sti
    ret

idt_load:
    push ebp
    mov ebp, esp                ; reference
    push ebx
    mov ebx, [ebp+8]            ; load the address of the IDT descriptor into ebx (passed as argument)
                                ; ebx is the top of stack for function, ebx + 4 is the return address, 
                                ; ebx + 8 is the first argument (the IDT descriptor) for the function idt_load
    lidt [ebx]                  ; load the IDT using the lidt instruction
    pop ebx
    pop ebp
    ret

int21h:
    cli                         ; prevent nested interrupt calls
    pushad
    call int21h_handler
    popad
    sti                         ; re-enable interrupts
    iret                        ; interrupt return


no_interrupt:
    cli
    pushad
    call no_interrupt_handler
    popad
    sti
    iret