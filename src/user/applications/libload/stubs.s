
[global _libload_resolve_symbol]
[extern _libload_dofixup]

_libload_resolve_symbol:
    call _libload_dofixup
    ret
