[GLOBAL resolveSymbol]
resolveSymbol:
  pop r15
  pop r14

  push rbx
  push rdx
  push rcx
        
  mov rbx, r15          ; First parameter.
  mov rdx, r14          ; Second parameter.

  mov rax, 1            ; KernelCoreSyscallManager::link
  mov r13, rsp
  syscall               ; Syscall.

  pop rcx
  pop rdx
  pop rbx               ; Get rbx back.
        
  jmp rax
