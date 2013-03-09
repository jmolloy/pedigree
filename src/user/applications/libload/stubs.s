
[global _libload_resolve_symbol]
[extern _libload_dofixup]

_libload_resolve_symbol:
    call _libload_dofixup
    add esp, 8
    jmp eax
