; Copyright (c) 2008-2014, Pedigree Developers
; 
; Please see the CONTRIB file in the root of the source tree for a full
; list of contributors.
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

[bits 64]
[section .text]

[GLOBAL sigret_stub]
[GLOBAL sigret_stub_end]

; void sigret_stub(size_t p1, size_t p2, size_t p3, size_t p4);
sigret_stub:

  ; Grab the handler address
  mov rax, rdi

  ; Check for serialisation buffer
  xor r8, r8
  cmp rsi, 0
  jz .justRun
  mov r8, [rsi]
  and r8, 0xff
  
  ; TODO: Push the handler address too and call a common stub of some sort...

.justRun:

  ; First parameter: event number.
  mov rdi, r8

  ; Third parameter: buffer.
  mov rdx, rsi

  ; Second parameter: handler address.
  mov rsi, rax

  ; Call the handler
  call rax
  
  ; Return to the kernel
  mov rax, 0x10065
  syscall
  
  jmp $
  
sigret_stub_end:

[GLOBAL pthread_stub]
[GLOBAL pthread_stub_end]

; void pthread_stub(size_t p1, size_t p2, size_t p3, size_t p4);
pthread_stub:
  
  ; Call the entry function
  mov rbx, 0
  mov rax, 0x10051
  syscall

  ; First parameter: entry point.
  mov rax, rdi

  ; Second parameter: argument.
  mov rdi, rsi
  
  ; Run the function
  call rax
  
  ; Effectively, pthread_exit... We can't refer to libc functions from here,
  ; as this code is copied to userspace at runtime.
  mov rbx, rax
  mov rax, 0x10050
  syscall
  
  jmp $

pthread_stub_end:
