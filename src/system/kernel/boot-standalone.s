;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; boot-standalone.s - Used when we are not relying on the bootloader.

MBOOT_PAGE_ALIGN   equ 1<<0
MBOOT_MEM_INFO     equ 1<<1
MBOOT_HEADER_MAGIC equ 0x1BADB002
MBOOT_HEADER_FLAGS equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM     equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 32]

[EXTERN code]
[EXTERN bss]
[EXTERN end]

mboot:
  dd 0
  dd MBOOT_HEADER_MAGIC
  dd MBOOT_HEADER_FLAGS
  dd MBOOT_CHECKSUM
  dd mboot
  dd code
  dd bss
  dd end
  dd start

[GLOBAL start]
[EXTERN _main]

[SECTION .text]
start:
  cli

  ; Enable PSE (Page Size Extension)
  mov eax, cr4
  or eax, 0x10
  mov cr4, eax

  mov eax, 0x83
  mov [pagedirectory], eax
  add eax, pagedirectory
  mov [pagedirectory + 0xFFC], eax

  mov eax, pagedirectory
  mov cr3, eax

  mov eax, cr0
  or eax, 0x80000000
  mov cr0, eax

  mov eax, 0x83
  mov [pagedirectory + 4], eax
  mov eax, cr3
  mov cr3, eax

  push ebx
  ; clear the stackframe
  xor ebp, ebp
  call _main
  jmp $

[SECTION .bss]
pagedirectory:
  resb 4096
