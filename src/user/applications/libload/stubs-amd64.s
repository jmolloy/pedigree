
[global _libload_resolve_symbol]
[extern _libload_dofixup]

_libload_resolve_symbol:
    pop r15
    pop r14

    push rdi
    push rsi

    mov rdi, r15
    mov rsi, r14
    call _libload_dofixup

    pop rsi
    pop rdi

    jmp rax
