;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; boot-standalone.s - Used when we are not relying on the bootloader.

MBOOT_PAGE_ALIGN   equ 1<<0
MBOOT_MEM_INFO     equ 1<<1
MBOOT_HEADER_MAGIC equ 0x1BADB002
MBOOT_HEADER_FLAGS equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM     equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 64]

align 4
mboot:
  dd MBOOT_HEADER_MAGIC
  dd MBOOT_HEADER_FLAGS
  dd MBOOT_CHECKSUM

[GLOBAL start]
[GLOBAL pml4]
[EXTERN _main]
[EXTERN code]
[EXTERN end]

[SECTION .text]
KERNEL_BASE        equ 0xFFFFFFFF7FF00000
start:
  cli
  push rbx

  ; Map a page directory pointer table for 0-256GB and the upmost 256GB and
  ; for the pyhsical memory mapping
  mov rax, pml4 - KERNEL_BASE
  mov rbx, pagedirectorypointer0 - KERNEL_BASE + 0x03
  mov [rax], rbx
  mov rbx, pagedirectorypointer1 - KERNEL_BASE + 0x03
  mov [rax + 0xFF8], rbx
  mov rbx, pagedirectorypointer2 - KERNEL_BASE + 0x03
  mov [rax + 0x800], rbx

  ; Map a page directory for 0-2GB and the upper 2GB and for the physical
  ; memory mapping
  mov rax, pagedirectorypointer0 - KERNEL_BASE
  mov rbx, pagedirectory0 - KERNEL_BASE + 0x03
  mov [rax], rbx

  mov rax, pagedirectorypointer1 - KERNEL_BASE
  mov rbx, pagedirectory1 - KERNEL_BASE + 0x03
  mov [rax + 0xFF0], rbx
  mov rbx, pagedirectory2 - KERNEL_BASE + 0x03
  mov [rax + 0xFF8], rbx

  mov rax, pagedirectorypointer2 - KERNEL_BASE
  mov rbx, pagedirectory3 - KERNEL_BASE + 0x03
  mov [rax], rbx

  ; Identity map 0-2MB
  mov rax, pagedirectory0 - KERNEL_BASE
  mov qword [rax], 0x83

  ; Map the page table for the kernel
  mov rax, pagedirectory1 - KERNEL_BASE
  mov rbx, pagetable0 - KERNEL_BASE + 0x03
  mov [rax], rbx

  ; Map the kernel
  mov rax, pagetable0 - KERNEL_BASE + 0x08
  mov rbx, code
  mov rcx, end
  mov r8, KERNEL_BASE
  .mapkernel:
    mov rdx, rbx
    sub rdx, r8
    or rdx, 0x03
    mov [rax], rdx
    add rax, 0x08
    add rbx, 0x1000
    cmp rbx, rcx
    jne .mapkernel

  ; Map the page table for the stack
  mov rax, pagedirectory2 - KERNEL_BASE
  mov rbx, pagetable1 - KERNEL_BASE + 0x03
  mov [rax + 0xFF8], rbx

  ; Map the kernel stack
  mov rax, pagetable1 - KERNEL_BASE
  mov rbx, stack - KERNEL_BASE + 0x03
  mov [rax + 0xFF8], rbx
  add rbx, 4096
  mov [rax + 0xFF0], rbx
  add rbx, 4096
  mov [rax + 0xFE8], rbx
  add rbx, 4096
  mov [rax + 0xFE0], rbx
  add rbx, 4096
  mov [rax + 0xFD8], rbx
  add rbx, 4096
  mov [rax + 0xFD0], rbx
  add rbx, 4096
  mov [rax + 0xFC8], rbx
  add rbx, 4096
  mov [rax + 0xFC0], rbx

  ; Map the lower 1GB of physical memory to 0xFFFF800000000000
  mov rax, pagedirectory3 - KERNEL_BASE
  mov rbx, 0x83
  mov rcx, 512
  .mapphysical:
    mov [rax], rbx
    add rbx, 0x200000
    add rax, 0x08
    loop .mapphysical

  ; Enable EFER.NXE
  mov ecx, 0xC0000080
  rdmsr
  or eax, 0x800
  wrmsr

  ; Switch to the new address space
  mov rax, pml4 - KERNEL_BASE
  mov cr3, rax

  pop rdi
  mov rsp, 0

  ; clear the stackframe
  xor rbp, rbp

  mov rax, GDTR
  lgdt [rax]

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
  mov rax, callmain
  push rax
  iretq

callmain:
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  call _main
  jmp $

[SECTION .data]
  GDTR:
    dw 23
    dq GDT
  GDT:
    dq 0
    dd 0
    db 0
    db 0x98
    db 0x20
    db 0
    dd 0
    db 0
    db 0x92
    dw 0

[SECTION .bss]
align 4096
pml4:
  resb 4096

; Page directory pointer table for 0-256GB
pagedirectorypointer0:
  resb 4096

; Page directory pointer table for the upmost 256GB
pagedirectorypointer1:
  resb 4096

; Page directory pointer table for the physical memory mapping
pagedirectorypointer2:
  resb 4096

; Page directory for the 0-1GB
pagedirectory0:
  resb 4096

; Page directory for the kernel code/data
pagedirectory1:
  resb 4096

; Page directory for the kernel stack
pagedirectory2:
  resb 4096

; Page directory for the physical memory mapping
pagedirectory3:
  resb 4096

; Page table for the kernel code/data
pagetable0:
  resb 4096

; Page table for the kernel stack
pagetable1:
  resb 4096

; The kernel stack
stack:
  resb 8192
  resb 8192
  resb 8192
  resb 8192
