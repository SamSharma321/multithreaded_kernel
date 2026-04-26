section .asm

global insb
global insw
global outb
global outw

insb:
    push ebp            ; store base pointer on the stack
    mov ebp, esp        ; point the base pointer to current stack frame -> function call to access instructions, parameters and return address
    
    xor eax, eax        ; make eax 0
    mov edx, [ebp + 8]
    in al, dx           ; lower 8 bits of eax will be set -> al i spart of eax register - [ah:al]

    pop ebp             ; restore the base pointer stacked initially
    ret 

insw:
    push ebp
    mov ebp, esp

    xor eax, eax
    mov edx, [ebp + 8]
    in ax, dx

    pop ebp
    ret

outb:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 12]
    mov edx, [ebp + 8]
    out dx, al

    pop ebp
    ret

outw:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 12]
    mov edx, [ebp + 8]
    out dx, ax

    pop ebp
    ret