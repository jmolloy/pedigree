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
[EXTERN init]
[EXTERN end]

[SECTION .init.text]
KERNEL_BASE        equ 0xBFF00000
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
  mov eax, 0x03 + pagetable1 - KERNEL_BASE
  mov [pagedirectory - KERNEL_BASE + 0xC00], eax

  ;Map the page-tables to 4GB - 4MB (the last 4MB)
  mov eax, 0x03 + pagedirectory - KERNEL_BASE
  mov [pagedirectory - KERNEL_BASE + 0xFFC], eax

  ; Map the kernel
  mov eax, pagetable1 - KERNEL_BASE
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

  ;Map the pagetable0 for stack and the page-directory to 4GB - 12MB
  mov eax, 0x03 + pagetable0 - KERNEL_BASE
  mov [pagedirectory - KERNEL_BASE + 0xFF4], eax

  ;Map the stack and the page-directory into pagetable0
  mov eax, 0x03 + stack - KERNEL_BASE
  mov [pagetable0 - KERNEL_BASE + 0xFF4], eax
  add eax, 4096
  mov [pagetable0 - KERNEL_BASE + 0xFF0], eax
  add eax, 4096
  mov [pagetable0 - KERNEL_BASE + 0xFEC], eax
  add eax, 4096
  mov [pagetable0 - KERNEL_BASE + 0xFE8], eax

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
  mov esp, 0xFF7FDC00

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
pagetable1:
  times 4096 db 0
stack:
  times 16384 db 0
