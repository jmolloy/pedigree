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

; X64GdtManager::loadSegmentRegisters
global _ZN13X64GdtManager20loadSegmentRegistersEv

_ZN13X64GdtManager20loadSegmentRegistersEv:
  mov rax, rsp
  sub rsp, 15
  mov r11, 0xFFFFFFFFFFFFFFF0
  and rsp, r11
  mov r11, 0x10
  push r11
  push rax
  pushf
  mov rax, 0x08
  push rax
  mov rax, here
  push rax
  iretq

  here:
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  ret
