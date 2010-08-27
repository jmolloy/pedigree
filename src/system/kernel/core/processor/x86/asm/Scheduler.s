;
; Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

; bool Processor::saveState(SchedulerState &)
global _ZN9Processor9saveStateER17X86SchedulerState
; void Processor::restoreState(SchedulerState &, volatile uintptr_t *)
global _ZN9Processor12restoreStateER17X86SchedulerStatePVm
; void Processor::restoreState(volatile uintptr_t *, SyscallState &)
global _ZN9Processor12restoreStateER17X86InterruptStatePVm
; void Processor::jumpKernel(volatile uintptr_t *, uintptr_t, uintptr_t,
;                            uintptr_t, uintptr_t, uintptr_t, uintptr_t)
global _ZN9Processor10jumpKernelEPVmmmmmmm
; void Processor::jumpUser(volatile uintptr_t *, uintptr_t, uintptr_t,
;                          uintptr_t, uintptr_t, uintptr_t, uintptr_t)
global _ZN9Processor8jumpUserEPVmmmmmmm
; void PerProcessorScheduler::deleteThreadThenRestoreState(Thread*, SchedulerState&)
global _ZN21PerProcessorScheduler28deleteThreadThenRestoreStateEP6ThreadR17X86SchedulerState

; void Thread::threadExited()
extern _ZN6Thread12threadExitedEv
; void PerProcessorScheduler::deleteThread(Thread *)
extern _ZN21PerProcessorScheduler12deleteThreadEP6Thread

[bits 32]
[section .text]


_ZN9Processor9saveStateER17X86SchedulerState:
    ;; Load the state pointer.
    mov     eax, [esp+4]

    ;; Save the stack pointer (without the return address)
    mov     ecx, esp
    add     ecx, 4

    ;; Save the return address.
    mov     edx, [esp]

    mov     [eax+0], edi
    mov     [eax+4], esi
    mov     [eax+8], ebx
    mov     [eax+12], ebp
    mov     [eax+16], ecx
    mov     [eax+20], edx

    ;; Save FPU data(if used)
    mov byte[eax+24], 0
    mov     edx, cr0
    and     edx, 8
    cmp     edx, 8
    je      .no_fpu
    mov byte[eax+24], 1



.no_fpu:

    ;; Return false.
    xor     eax, eax
    ret

_ZN9Processor12restoreStateER17X86SchedulerStatePVm:
    ;; Load the state pointer.
    mov     eax, [esp+4]

    ;; Load the lock pointer.
    mov     ecx, [esp+8]

    ;; Check for FPU use
    cmp byte[eax+24], 1
    je      .uses_fpu
    mov     edx, cr0
    or      edx, 8
    mov     cr0, edx
    jmp     .fin_fpu
.uses_fpu:
    mov     edx, cr0
    and     edx, 0xfffffff7
    mov     cr0, edx
    mov     edx, [esp+4]
    
    
.fin_fpu:

    ;; Reload all callee-save registers.
    mov     edi, [eax+0]
    mov     esi, [eax+4]
    mov     ebx, [eax+8]
    mov     ebp, [eax+12]
    mov     esp, [eax+16]
    mov     edx, [eax+20]
    ;; Ignore lock if none given (ecx == 0).
    cmp     ecx, 0
    jz      .no_lock
    ;; Release lock.
    mov     dword [ecx], 1
.no_lock:

    ;; Return true.
    mov     eax, 1

    ;; Push the return address on to the stack before returning.
    push    edx
    ret

_ZN9Processor12restoreStateER17X86InterruptStatePVm:
    ;; Load the lock pointer.
    mov     ecx, [esp+8]

    ;; The state pointer is on this thread's kernel stack, so change to it.
    mov     eax, [esp+4]
    mov     esp, [eax]

    ;; Stack changed, now we can unlock the old thread.
    cmp     ecx, 0
    jz      .no_lock
    ;; Release lock.
    mov     dword [ecx], 1
.no_lock:

    ;; Restore the registers
    pop ds
    mov ax, ds
    mov es, ax
    popa

    ;; Remove the errorcode and the interrupt number from the stack
    add esp, 0x08
    iret

_ZN9Processor10jumpKernelEPVmmmmmmm:
    ;; Load the lock pointer.
    mov     ecx, [esp+4]

    ;; Load the address to jump to.
    mov     eax, [esp+8]
    ;; Load the new stack pointer.
    mov     ebx, [esp+12]
    ;; And each parameter.
    mov     edx, [esp+16]
    mov     esi, [esp+20]
    mov     edi, [esp+24]
    mov     ebp, [esp+28]

    ;; Change stacks.
    cmp     ebx, 0
    jz      .no_stack_change
    mov     esp, ebx
.no_stack_change:

    ;; Stack changed, now we can unlock the old thread.
    cmp     ecx, 0
    jz      .no_lock
    ;; Release lock.
    mov     dword [ecx], 1
.no_lock:

    ;; Set up the stack - push parameters first.
    push    ebp
    push    edi
    push    esi
    push    edx
    ;; Then the return address.
    push    dword _ZN6Thread12threadExitedEv

    ;; New stack frame.
    xor     ebp, ebp
    ;; Enable interrupts and jump.
    sti
    jmp     eax

_ZN9Processor8jumpUserEPVmmmmmmm:
    ;; Load the lock pointer.
    mov     ecx, [esp+4]

    ;; Load the address to jump to.
    mov     eax, [esp+8]
    ;; Load the new stack pointer.
    mov     ebx, [esp+12]
    ;; And each parameter.
    mov     edx, [esp+16]
    mov     esi, [esp+20]
    mov     edi, [esp+24]
    mov     ebp, [esp+28]

    ;; Change stacks.
    mov     esp, ebx

    ;; Stack changed, now we can unlock the old thread.
    cmp     ecx, 0
    jz      .no_lock
    ;; Release lock.
    mov     dword [ecx], 1
.no_lock:

    ;; Set up the stack - push parameters first.
    push    ebp
    push    edi
    push    esi
    push    edx
    ;; Then the return address. User mode threads can't return to kernel code,
    ;; so just make it 0.
    push    dword 0xbeef

    ;; New stack frame.
    xor     ebp, ebp

    ; Load all segment descriptors with the user-mode selector.
    mov    bx, 0x23
    mov    ds, bx
    mov    es, bx
    
    ; Load FS/GS with the TLS selector
    mov    bx, 0x33
    mov    fs, bx
    mov    gs, bx

    ; Save a copy of the stack pointer in its current position - this will
    ; become the target stack.
    mov    ecx, esp
    ; Stack segment (SS)
    push   0x23
    ; Stack pointer
    push   ecx

    ; EFLAGS: Push to stack, pop, OR in the IF flag (interrupt enable, 0x200) and
    ; push again.
    pushfd
    pop    ecx
    or     ecx, 0x200
    push   ecx

    ; Code segment (CS)
    push   0x1B
    ; EIP
    push   eax

    ; Interrupt-return to the start function.
    iret

_ZN21PerProcessorScheduler28deleteThreadThenRestoreStateEP6ThreadR17X86SchedulerState:
    ;; Load the state pointer.
    mov     edi, [esp+8]

    ;; Load the Thread* pointer.
    mov     ecx, [esp+4]

    ;; Reload ESP and call deleteThread from the new stack.
    mov     esp, [edi+16]
    push    ecx
    call    _ZN21PerProcessorScheduler12deleteThreadEP6Thread
    pop     ecx

    ;; Move the state pointer from a callee-save register to a scratch register.
    mov     eax, edi

    ;; Reload all callee-save registers.
    mov     edi, [eax+0]
    mov     esi, [eax+4]
    mov     ebx, [eax+8]
    mov     ebp, [eax+12]
    mov     esp, [eax+16]
    mov     edx, [eax+20]

    ;; Return true.
    mov     eax, 1

    ;; Push the return address on to the stack before returning.
    push    edx
    ret
