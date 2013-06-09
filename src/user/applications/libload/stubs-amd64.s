
[global _libload_resolve_symbol]
[extern _libload_dofixup]

_libload_resolve_symbol:
    ; pop r15
    ; pop r14

    ; Save caller-save registers.
    push rcx
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11

    mov rdi, [rsp + 64]
    mov rsi, [rsp + 72]
    call _libload_dofixup

    ; Restore caller-save registers.
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx

    add rsp, 16
    jmp rax
