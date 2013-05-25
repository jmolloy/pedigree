;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; boot-standalone.s - Used when we are not relying on the bootloader.

MBOOT_PAGE_ALIGN   equ 1<<0
MBOOT_MEM_INFO     equ 1<<1
MBOOT_HEADER_MAGIC equ 0x1BADB002
MBOOT_HEADER_FLAGS equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM     equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 64]

[SECTION .init.multiboot]
align 4
mboot:
  dd MBOOT_HEADER_MAGIC
  dd MBOOT_HEADER_FLAGS
  dd MBOOT_CHECKSUM

[GLOBAL start]
[GLOBAL pml4]
[EXTERN _main]
[EXTERN init]
[EXTERN end]

[SECTION .init.text]
KERNEL_BASE        equ 0xFFFFFFFF7FF00000
start:
  cli
  push rbx

  ; Map a page directory pointer table for 0-512GB and the upmost 512GB and
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
  add rbx, 0x1000
  mov [rax + 8], rbx
  add rbx, 0x1000
  mov [rax + 16], rbx
  add rbx, 0x1000
  mov [rax + 24], rbx

  ; Identity map 0-4MB
  mov rax, pagedirectory0 - KERNEL_BASE
  mov qword [rax], 0x83
  add rax, 0x08
  mov qword [rax], 0x200083

  ; Map the page table for the kernel
  mov rax, pagedirectory1 - KERNEL_BASE
  mov rbx, pagetable0 - KERNEL_BASE + 0x03
  mov [rax], rbx
  mov rbx, pagetable1 - KERNEL_BASE + 0x03
  mov [rax+0x08], rbx
        
  ; Map the kernel
  mov rax, pagetable0 - KERNEL_BASE + 0x08
  mov rbx, init
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
  mov rbx, pagetable2 - KERNEL_BASE + 0x03
  mov [rax + 0xFF8], rbx

  ; Map the kernel stack
  mov rax, pagetable2 - KERNEL_BASE
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

  ; Map the lower 4GB of physical memory to 0xFFFF800000000000
  mov rax, pagedirectory3 - KERNEL_BASE
  mov rbx, 0x83
  mov rcx, 4 * 512
  .mapphysical:
    mov [rax], rbx
    add rbx, 0x200000
    add rax, 0x08
    loop .mapphysical

  ; Enable EFER.NXE
  ; TODO: Do we nede to check for NXE support first?
  mov ecx, 0xC0000080
  rdmsr
  or eax, 0x800
  wrmsr

  ; Switch to the new address space
  mov rax, pml4 - KERNEL_BASE
  mov cr3, rax

  pop rdi
  mov rsp, stack
  add rsp, 0x10000

  ; clear the stackframe
  xor rbp, rbp

  mov rax, GDTR
  lgdt [rax]

  xor rax, rax
  mov r11, 0x10
  push r11
  push rax
  pushfq
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
  
  ; Get the FPU and SSE going
  call ___startup_init_fpu_sse

  call _main
  jmp $

; Enables the FPU and SSE (used very early by GCC's codegen)
___startup_init_fpu_sse:
  push rbp
  mov rbp, rsp
  
  ; One scratch variable
  sub rsp, 8
  
  ; Grab CPUID
  mov rax, 1
  xor rcx, rcx
  cpuid
  
  test rdx, 1
  jz .nofpu
  
  ; FPU is available, start it up
  mov rbx, cr0
  or rbx, 0x22 ; NE | MP
  and rbx, 0xFFFFFFFFFFFFFFF3 ; Remove EM and TS
  
  mov cr0, rbx
  
  finit
  
  mov qword [rsp], 0x33F
  fldcw [rsp]
  
  mov cr0, rbx
  
  .nofpu:
  
  ; We support FXSAVE/FXRSTOR, sort of. Needed for SSE - NMFaultHandler will do
  ; this properly later.
  test rdx, (1 << 24)
  jz .nofxsr
  
  ; Enable it
  mov rbx, cr4
  or rbx, (1 << 9)
  mov cr4, rbx
  
  .nofxsr:
  
  ; SSE?
  test rdx, (1 << 25) ; SSE feature
  jz .nosse
  
  ; SSE is available, start it up
  mov rbx, cr4
  or rbx, (1 << 10) ; SIMD floating-point exception handling bit
  mov cr4, rbx
  
  stmxcsr [rsp]
  mov rbx, [rsp]
  
  ; Mask all exceptions
  or rbx, (1 << 12) | (1 << 11) | (1 << 10) | (1 << 9) | (1 << 8) | (1 << 7)
  and rbx, 0xFFFFFFFFFFFFFFC0 ; Clear any pending exceptions
  
  ; Set rounding method
  or rbx, (3 << 13)
  
  mov [rsp], rbx
  
  ; Write the control word
  ldmxcsr [rsp]
  
  .nosse:
  
  ; TS bit
  ; mov rax, cr0
  ; or rax, (1 << 3)
  ; mov cr0, rax
  
  mov rsp, rbp
  pop rbp
  ret

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

[SECTION .asm.bss nobits]
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

; Page directories for the physical memory mapping
pagedirectory3:
  resb 4096
  resb 4096
  resb 4096
  resb 4096

; Page table for the kernel code/data (0MB .. 2MB)
pagetable0:
  resb 4096

; Page table for the kernel code/data (2MB .. 4MB)
pagetable1:
  resb 4096

; Page table for the kernel stack
pagetable2:
  resb 4096
        
; The kernel stack
stack:
  resb 0x10000
