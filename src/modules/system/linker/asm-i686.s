[GLOBAL resolveSymbol]
resolveSymbol:
  pop edx               ; First parameter - will go into ebx once that's saved.
  pop ecx               ; Second parameter
  push ebx
  mov ebx, edx          ; First parameter.

  mov eax, 1            ; KernelCoreSyscallManager::link

  int 255               ; Syscall.
  pop ebx               ; Get ebx back.
  jmp eax
