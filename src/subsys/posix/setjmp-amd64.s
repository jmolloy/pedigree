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

; This is file is a merger of SETJMP.S and LONGJMP.S;/
;
;  This file was modified to use the __USER_LABEL_PREFIX__ and
;  __REGISTER_PREFIX__ macros defined by later versions of GNU cpp by
;  Joel Sherrill (joel@OARcorp.com)
;  Slight change: now includes i386mach.h for this (Werner Almesberger)
;
; Copyright (C) 1991 DJ Delorie
; All rights reserved.
;
; Redistribution and use in source and binary forms is permitted
; provided that the above copyright notice and following paragraph are
; duplicated in all such forms.
;
; This file is distributed WITHOUT ANY WARRANTY; without even the implied
; warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
;
 
; Copyright (c) 2010 Matthew Iselin
; Reworked for Pedigree and nasm.
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

;  jmp_buf:
;   rbx rbp r12 r13 r14 r15 rsp rip
;   0   8   16  24  32  40  48  56

global setjmp:function
global longjmp:function

global _setjmp:function
global _longjmp:function

; C function that runs a system call to check to see if we are in a signal
; context, and unwind that context. Allows longjmp from signal handlers.
extern __pedigree_revoke_signal_context

setjmp:
  ; Grab return address.
  pop rsi

  ; Save registers.
  mov [rdi + 0], rbx
  mov [rdi + 8], rbp
  mov [rdi + 16], r12
  mov [rdi + 24], r13
  mov [rdi + 32], r14
  mov [rdi + 40], r15

  ; Stack after return (as longjmp takes us to the calling frame).
  mov [rdi + 48], rsp
  ; Stack fixup now that we have saved RSP.
  push rsi

  ; Return address.
  mov [rdi + 56], rsi

  ; Return 0 - saved.
  xor rax, rax
  ret

longjmp:
  ; Revoke any existing signal context (in case we are doing a longjmp
  ; out of a signal handler.
  push rdi ; Preserve RDI/RSI across function call & syscall.
  push rsi
  call [rel __pedigree_revoke_signal_context wrt ..got]
  pop rsi
  pop rdi

  ; Return value.
  mov rax, rsi

  ; Restore registers and stack.
  mov rbx, [rdi + 0]
  mov rbp, [rdi + 8]
  mov r12, [rdi + 16]
  mov r13, [rdi + 24]
  mov r14, [rdi + 32]
  mov r15, [rdi + 40]
  mov rsp, [rdi + 48]

  ; Jump to saved location.
  mov rdx, [rdi + 56]
  jmp rdx

_setjmp:
    jmp setjmp

_longjmp:
    jmp longjmp

