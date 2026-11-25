[bits 32]

global switch_context
global task_trampoline
extern task_trampoline_c

section .text

; void switch_context(uint32_t* old_esp, uint32_t new_esp)
switch_context:
    mov eax, [esp + 4]    ; old_esp
    mov edx, [esp + 8]    ; new_esp

    push ebx
    push esi
    push edi
    push ebp

    mov [eax], esp        ; salva ESP corrente in *old_esp
    mov esp, edx          ; carica nuovo stack pointer

    pop ebp
    pop edi
    pop esi
    pop ebx
    ret

task_trampoline:
    ; all'ingresso qui, [esp] è già l'argomento `launch` (TaskLaunchArgs*)
    call task_trampoline_c
    hlt

section .note.GNU-stack noalloc noexec nowrite progbits