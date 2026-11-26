section .multiboot
align 4
    MULTIBOOT_MAGIC     equ 0x1BADB002
    MULTIBOOT_FLAGS     equ (1 << 2)        ; Bit 2: richiedi informazioni video (framebuffer)
    MULTIBOOT_CHECKSUM  equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

    dd 0 ; header_addr
    dd 0 ; load_addr
    dd 0 ; load_end_addr
    dd 0 ; bss_end_addr
    dd 0 ; entry_addr

    ; Campi video (offset 32)
    dd 0    ; mode_type: 0 = modalità grafica lineare
    dd 1024 ; width
    dd 768  ; height
    dd 32   ; depth (bpp)

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
    push ebx
    push eax
    call kernel_main           ; Entra nel kernel C++
    add esp, 8

    cli
.hang:
    hlt                        ; Ferma la CPU
    jmp .hang