;
; Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
;
; Permission to use, copy, modify, and distribute this software for any
; purpose with or without fee is hereby granted, provided that the above
; copyright notice and this permission notice appear in all copies.
;
; THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
; WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
; ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
; WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
; ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
; OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
;

; X64SyscallManager::syscall(SyscallState &syscallState)
extern _ZN17X64SyscallManager7syscallER15X64SyscallState

; Export the syscall handler
global syscall_handler

;##############################################################################
;### Code section #############################################################
;##############################################################################
[bits 64]
[section .text]

;##############################################################################
;### assembler stub for syscalls ##############################################
;##############################################################################
; TODO: we might want to use the highest (or 8byte lower, to allow rsp saving) 8byte
;       of gs.base to save the gs.base value, interrupts would just ignore this value
syscall_handler:
  ; Load kernel stack into gs base
  swapgs

  ; Save the registers
  mov [gs: -0x08], rsp      ; rsp
  mov [gs: -0x10], rcx      ; rip/rcx
  mov [gs: -0x18], r11      ; rflags/r11
  mov [gs: -0x20], rax      ; rax
  mov [gs: -0x28], rbx      ; rbx
  mov [gs: -0x30], rdx      ; rdx
  mov [gs: -0x38], rdi      ; rdi
  mov [gs: -0x40], rsi      ; rsi
  mov [gs: -0x48], rbp      ; rbp
  mov [gs: -0x50], r8       ; r8
  mov [gs: -0x58], r9       ; r9
  mov [gs: -0x60], r10      ; r10
  mov [gs: -0x68], r12      ; r12
  mov [gs: -0x70], r13      ; r13
  mov [gs: -0x78], r14      ; r14
  mov [gs: -0x80], r15      ; r15

  ; Restore the original gs base
  swapgs

  ; Switch to the kernel stack
  mov rcx, 0xC0000102
  rdmsr
  mov esp, edx
  shl rsp, 32
  mov rcx, 0xFFFFFFFF
  and rax, rcx
  add rsp, rax
  sub rsp, 0x80

  ; Call the C++ handler function
  mov rdi, rsp
  call _ZN17X64SyscallManager7syscallER15X64SyscallState

  pop r15
  pop r14
  pop r13
  pop r12
  pop r10
  pop r9
  pop r8
  pop rbp
  pop rsi
  pop rdi
  pop rdx
  pop rbx
  pop rax
  pop r11
  pop rcx
  pop rsp

  sysret
