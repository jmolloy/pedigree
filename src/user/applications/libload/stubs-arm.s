# Copyright (c) 2008-2014, Pedigree Developers
# 
# Please see the CONTRIB file in the root of the source tree for a full
# list of contributors.
# 
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

.global _libload_resolve_symbol
.extern _libload_dofixup

_libload_resolve_symbol:
    b .

#    ; pop r15
#    ; pop r14
#
#    ; Save caller-save registers.
#    push rcx
#    push rdx
#    push rdi
#    push rsi
#    push r8
#    push r9
#    push r10
#    push rax
#
#    mov rdi, [rsp + 64]
#    mov rsi, [rsp + 72]
#    call _libload_dofixup
#    mov r11, rax
#
#    ; Restore caller-save registers.
#    pop rax
#    pop r10
#    pop r9
#    pop r8
#    pop rsi
#    pop rdi
#    pop rdx
#    pop rcx
#
#    add rsp, 16
#    jmp r11

