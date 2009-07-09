[GLOBAL resolveSymbol]
resolveSymbol:
  pop rdx               ; First parameter - will go into rbx once that's saved.
  pop rcx               ; Second parameter
  push rbx
  mov rbx, rdx          ; First parameter.

  mov rax, 1            ; KernelCoreSyscallManager::link

  int 255               ; Syscall.
  pop rbx               ; Get rbx back.
  jmp rax
