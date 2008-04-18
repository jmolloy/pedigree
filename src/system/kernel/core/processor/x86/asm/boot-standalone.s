;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; boot-standalone.s - Used when we are not relying on the bootloader.

MBOOT_PAGE_ALIGN   equ 1<<0
MBOOT_MEM_INFO     equ 1<<1
MBOOT_HEADER_MAGIC equ 0x1BADB002
MBOOT_HEADER_FLAGS equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM     equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 32]

[SECTION .init.multiboot]
align 4
mboot:
  dd MBOOT_HEADER_MAGIC
  dd MBOOT_HEADER_FLAGS
  dd MBOOT_CHECKSUM

[GLOBAL start]
[EXTERN _main]

[SECTION .init.text]
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
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFD4], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFD0], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFCC], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFC8], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFC4], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFC0], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFBC], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFB8], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFB4], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFB0], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFAC], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFA8], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFA4], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xFA0], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xF9C], eax
  add eax, 4096
  mov [pagetable0 - 0xC0000000 + 0xF98], eax

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

[SECTION .asm.bss]
global pagedirectory
pagedirectory:
  times 4096 db 0
pagetable0:
  times 4096 db 0
stack:
  times 98304 db 0
