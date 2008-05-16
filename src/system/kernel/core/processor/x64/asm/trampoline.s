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

  ; Set cr4.PAE
  mov eax, cr4
  or eax, 0x20
  mov cr4, eax

  ; Set Cr3
  mov eax, [0x7FF8]
  mov cr3, eax

  ; Test for EFER.NXE
  mov eax, 0x80000001
  cpuid
  and edx, 0x100000
  jz pmode1
  mov [efer], dword 0x901

pmode1:
  ; Set EFER.LME & EFER.NXE & EFER.SCE
  mov ecx, 0xC0000080
  rdmsr
  or eax, [efer]
  wrmsr

  ; Enable Paging
  mov eax, cr0
  or eax, 0x80000000
  mov cr0, eax

  lgdt [GDTR64]
  jmp 0x08:longmode

[bits 64]
longmode:
  ; Load the stack
  mov rsp, [0x7FF0]

  ; Jump to the kernel's Multiprocessor::applicationProcessorStartup() function
  mov rax, [0x7FE8]
  jmp rax
;##########################################################################
;##### EFER                                                           #####
;##########################################################################
efer:
  dd 0x101
;##########################################################################
;##### Global descriptor table                                        #####
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
;##### Global descriptor table register                               #####
;##########################################################################
GDTR:
  dw 0x18
  dd GDT
;##########################################################################
;#### Global descriptor table register                                 ####
;##########################################################################
GDTR64:
  dw 0x18
  dq GDT64
;##########################################################################
;#### Global descriptor table 64 bit                                   ####
;##########################################################################
GDT64:
  dq 0
  ;##################################################################
  ;### loader code-segment descriptor 64-bit                     ####
  ;##################################################################
  dd 0      ; unused
  db 0      ; unused
  db 0x98
  db 0x20
  db 0      ; unused
  ;#################################################################
  ;### loader data-segment descriptor 64-bit                     ###
  ;#################################################################
  dd 0      ; unused
  db 0      ; unused
  db 0x90
  dw 0      ; unused
