[BITS 32]
global idt_load
global isr0
global isr13
global isr14
extern isr_handler

section .text
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

isr0:
    cli
    push dword 0
    push dword 0
    jmp isr_common

    isr13:
    cli
    push dword 13
    jmp isr_common

isr14:
    cli
    push dword 14
    jmp isr_common

isr_common:
    pushad
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popad
    add esp, 8
    iretd