;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; boot-standalone.s - Used when we are not relying on the bootloader.

MBOOT_PAGE_ALIGN   equ 1<<0
MBOOT_MEM_INFO     equ 1<<1
MBOOT_HEADER_MAGIC equ 0x1BADB002
MBOOT_HEADER_FLAGS equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM     equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 32]

[GLOBAL start]
[EXTERN _main]
[EXTERN init]
[EXTERN end]

; The Multiboot Header
[SECTION .init.multiboot]
align 4
mboot:
  dd MBOOT_HEADER_MAGIC
  dd MBOOT_HEADER_FLAGS
  dd MBOOT_CHECKSUM

[SECTION .init.text]
KERNEL_BASE        equ 0xFF300000
start:
  cli
  push ebx

  ; Enable PSE (Page Size Extension)
  mov eax, cr4
  or eax, 0x10
  mov cr4, eax

  ; Identity map the first 4MB 
  mov eax, 0x83
  mov [pagedirectory - KERNEL_BASE], eax

  ; Map the pagetable1 for stack
  mov eax, 0x03 + pagetable1 - KERNEL_BASE
  mov [pagedirectory - KERNEL_BASE + 0xFF0], eax

  ; Map the pagetable0 for the kernel image and the page-directory
  mov eax, 0x03 + pagetable0 - KERNEL_BASE
  mov [pagedirectory - KERNEL_BASE + 0xFF4], eax

  ; Map the page-tables
  mov eax, 0x03 + pagedirectory - KERNEL_BASE
  mov [pagedirectory - KERNEL_BASE + 0xFFC], eax

  ; Map the kernel
  mov eax, pagetable0 - KERNEL_BASE
  mov ebx, init
  mov ecx, end
  mov esi, KERNEL_BASE
  .mapkernel:
    mov edx, ebx
    sub edx, esi
    or edx, 0x03
    mov [eax], edx
    add eax, 0x04
    add ebx, 0x1000
    cmp ebx, ecx
    jne .mapkernel

  ; Map the stack and the page-directory into pagetable0
  mov eax, 0x03 + stack - KERNEL_BASE
  mov [pagetable1 - KERNEL_BASE + 0xFF8], eax
  add eax, 4096
  mov [pagetable1 - KERNEL_BASE + 0xFF4], eax
  add eax, 4096
  mov [pagetable1 - KERNEL_BASE + 0xFF0], eax
  add eax, 4096
  mov [pagetable1 - KERNEL_BASE + 0xFEC], eax
  add eax, 4096
  mov [pagetable1 - KERNEL_BASE + 0xFE8], eax
  add eax, 4096
  mov [pagetable1 - KERNEL_BASE + 0xFE4], eax
  add eax, 4096
  mov [pagetable1 - KERNEL_BASE + 0xFE0], eax
  add eax, 4096
  mov [pagetable1 - KERNEL_BASE + 0xFDC], eax

  ; Map the page-directory to 0xFF7FF000
  mov eax, 0x03 + pagedirectory - KERNEL_BASE
  mov [pagetable0 - KERNEL_BASE + 0xFFC], eax

  mov eax, pagedirectory - KERNEL_BASE
  mov cr3, eax

  mov eax, cr0
  or eax, 0x80000000
  mov cr0, eax

  ; Set stack
  pop ebx
  mov esp, 0xFF3FEC00

  push ebx
  ; clear the stackframe
  xor ebp, ebp
  mov eax, callmain
  jmp eax

callmain:
  call _main
  jmp $

[SECTION .asm.bss nobits]
global pagedirectory
pagedirectory: resb 4096
pagetable0:    resb 4096
pagetable1:    resb 4096
stack:         resb 32678
