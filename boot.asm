section .multiboot
align 4
    dd 0x1BADB002
    dd 0x00                    ; Flags
    dd -(0x1BADB002 + 0x00)    ; Checksum (magic + flags + checksum = 0)

section .bss
align 16
stack_bottom:
    resb 16384                 ; 16 KB stack
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    call kernel_main           ; Entra nel kernel C++

    cli
.hang:
    hlt                        ; Ferma la CPU
    jmp .hang