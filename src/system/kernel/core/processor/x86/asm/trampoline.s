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

;##############################################################################
;### Trampoline code ##########################################################
;##############################################################################
[bits 16]
[org 0x7000]
  hlt
  ; Load the new GDT
  lgdt [GDTR]

  ; Set Cr0.PE
  mov eax, cr0
  or eax, 0x01
  mov cr0, eax

  ; Jump into protected-mode
  jmp 0x08:pmode

[bits 32]
pmode:
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  ; load page directory
  mov eax, [0x7FFC]
  mov cr3, eax

  ; Enable PSE (=Page Size Extension)
  mov eax, cr4
  or eax, 0x10
  mov cr4, eax

  ; Enable Paging
  mov eax, cr0
  or eax, 0x80000000
  mov cr0, eax

  ; Load the stack
  mov esp, [0x7FF8]

  ; Jump to the kernel
  ; TODO
  hlt
  mov eax, [0x7FF4]
  jmp eax
;##########################################################################
;##### Global descriptor table ############################################
;##########################################################################
GDT:
  dd 0x00000000
  dd 0x00000000
  ; kernel-code
  dw 0xFFFF
  dw 0x0000
  db 0x00
  db 0x98
  db 0xCF
  db 0x00
  ; kernel-data
  dw 0xFFFF
  dw 0x0000
  db 0x00
  db 0x92
  db 0xCF
  db 0x00
;##########################################################################
;##### Global descriptor table register ###################################
;##########################################################################
GDTR:
  dw 0x18
  dd GDT
