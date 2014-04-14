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

;    jmp_buf:
;     eax ebx ecx edx esi edi ebp esp eip
;     0   4   8   12  16  20  24  28  32

global setjmp:function
global longjmp:function

global _setjmp:function
global _longjmp:function

; C function that runs a system call to check to see if we are in a signal
; context, and unwind that context. Allows longjmp from signal handlers.
extern __pedigree_revoke_signal_context

setjmp:

    push    ebp
    mov     ebp, esp

    push    edi
    mov     edi, [ebp + 8]

    mov     [edi + 0], eax
    mov     [edi + 4], ebx
    mov     [edi + 8], ecx
    mov     [edi + 12], edx
    mov     [edi + 16], esi

    mov     eax, [ebp - 4]
    mov     [edi + 20], eax

    mov     eax, [ebp + 0]
    mov     [edi + 24], eax

    mov     eax, esp
    add     eax, 12
    mov     [edi + 28], eax
    
    mov     eax, [ebp + 4]
    mov     [edi + 32], eax

    pop     edi
    mov     eax, 0
    leave
    ret

longjmp:
    push    ebp
    mov     ebp, esp
    
    ; Remove any existing signal context (behaviour undefined in nested signals)
    call __pedigree_revoke_signal_context

    mov     edi, [ebp + 8]  ; get jmp_buf 
    mov     eax, [ebp + 12] ; store retval in j->eax
    mov     [edi + 0], eax

    mov     ebp, [edi + 24]

    mov     esp, [edi + 28]
    
    push    dword [edi + 32]

    mov     eax, [edi + 0]
    mov     ebx, [edi + 4]
    mov     ecx, [edi + 8]
    mov     edx, [edi + 12]
    mov     esi, [edi + 16]
    mov     edi, [edi + 20]

    ret

_setjmp:
    call setjmp
    ret

_longjmp:
    call longjmp
    ret
