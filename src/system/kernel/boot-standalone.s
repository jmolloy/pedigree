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
  push ebx

  ; Enable PSE (Page Size Extension)
  mov eax, cr4
  or eax, 0x10
  mov cr4, eax

  ; Identity map the first 4MB 
  mov eax, 0x83
  mov [pagedirectory - 0xC0000000], eax
  mov [pagedirectory - 0xC0000000 + 0xC00], eax

  ;Map the page-tables to 4GB - 4MB (the last 4MB)
  mov eax, 0x03 + pagedirectory - 0xC0000000
  mov [pagedirectory - 0xC0000000 + 0xFFC], eax

  ;Map the pagetable0 for stack and the page-directory to 4GB - 8MB
  mov eax, 0x03 + pagetable0 - 0xC0000000
  mov [pagedirectory - 0xC0000000 + 0xFF8], eax

  ;Map the stack and the page-directory into pagetable0
  mov eax, 0x03 + stack - 0xC0000000
  mov [pagetable0 - 0xC0000000 + 0xFF4], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFF0], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFEC], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFE8], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFE4], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFE0], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFDC], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFD8], eax

  ; Map the page-directory to 0xFFBFF000
  mov eax, 0x03 + pagedirectory - 0xC0000000
  mov [pagetable0 - 0xC0000000 + 0xFFC], eax

  mov eax, pagedirectory - 0xC0000000
  mov cr3, eax

  mov eax, cr0
  or eax, 0x80000000
  mov cr0, eax

  ; Set stack
  pop ebx
  mov esp, 0xFFBFE000

  push ebx
  ; clear the stackframe
  xor ebp, ebp
  mov eax, callmain
  jmp eax

callmain:
  call _main
  jmp $

[SECTION .bss]
align 4096
global pagedirectory
pagedirectory:
  resb 4096
pagetable0:
  resb 4096
stack:
  resb 8192
  resb 8192
  resb 8192
  resb 8192
