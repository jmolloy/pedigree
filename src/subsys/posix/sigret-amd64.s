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
[bits 64]

[GLOBAL sigret_stub]
[GLOBAL sigret_stub_end]

; void sigret_stub(size_t p1, size_t p2, size_t p3, size_t p4);
sigret_stub:

  ; Grab the handler address
  mov rax, rdi
  
  ; First argument is the buffer
  mov rdi, rsi
  
  ; Call the handler
  call rax
  
  ; Return to the kernel
  mov rax, 0x10065
  syscall
  
  jmp $
  
sigret_stub_end:

[GLOBAL pthread_stub]
[GLOBAL pthread_stub_end]
[EXTERN pthread_exit]

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
