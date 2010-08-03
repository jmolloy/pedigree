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
global _ZN9Processor9saveStateER17X64SchedulerState
; void Processor::restoreState(SchedulerState &, volatile uintptr_t *)
global _ZN9Processor12restoreStateER17X64SchedulerStatePVm
; void Processor::restoreState(volatile uintptr_t *, SyscallState &)
global _ZN9Processor12restoreStateER15X64SyscallStatePVm
; void Processor::jumpKernel(volatile uintptr_t *, uintptr_t, uintptr_t,
;                            uintptr_t, uintptr_t, uintptr_t, uintptr_t)
global _ZN9Processor10jumpKernelEPVmmmmmmm
; void Processor::jumpUser(volatile uintptr_t *, uintptr_t, uintptr_t,
;                          uintptr_t, uintptr_t, uintptr_t, uintptr_t)
global _ZN9Processor8jumpUserEPVmmmmmmm
; void PerProcessorScheduler::deleteThreadThenRestoreState(Thread*, SchedulerState&)
global _ZN21PerProcessorScheduler28deleteThreadThenRestoreStateEP6ThreadR17X64SchedulerState

; void Thread::threadExited()
extern _ZN6Thread12threadExitedEv
; void PerProcessorScheduler::deleteThread(Thread *)
extern _ZN21PerProcessorScheduler12deleteThreadEP6Thread
        
[bits 64]
[section .text]

; [rdi] State pointer.
_ZN9Processor9saveStateER17X64SchedulerState:
    ;; Save the stack pointer (without the return address)
    mov     rax, rsp
    add     rax, 8

    ;; Save the return address.
    mov     rdx, [rsp]

    mov     [rdi+0], r8
    mov     [rdi+8], r9
    mov     [rdi+16], r10
    mov     [rdi+24], r11
    mov     [rdi+32], r12
    mov     [rdi+40], r13
    mov     [rdi+48], r14
    mov     [rdi+56], r15
    mov     [rdi+64], rbx
    mov     [rdi+72], rbp
    mov     [rdi+80], rax
    mov     [rdi+88], rdx

    ;; Return false.
    xor     rax, rax
    ret

; [rsi] Lock.
; [rdi] State pointer.
_ZN9Processor12restoreStateER17X64SchedulerStatePVm:
    ;; Reload all callee-save registers.
    mov     r8, [rdi+0]
    mov     r9, [rdi+8]
    mov     r10, [rdi+16]
    mov     r11, [rdi+24]
    mov     r12, [rdi+32]
    mov     r13, [rdi+40]
    mov     r14, [rdi+48]
    mov     r15, [rdi+56]
    mov     rbx, [rdi+64]
    mov     rbp, [rdi+72]
    mov     rsp, [rdi+80]
    mov     rdx, [rdi+88]

    ;; Ignore lock if none given (rcx == 0).
    cmp     rsi, 0
    jz      .no_lock
    ;; Release lock.
    mov     qword [rsi], 1
.no_lock:

    ;; Return true.
    xor     rax, rax
    inc     rax

    ;; Push the return address on to the stack before returning.
    push    rdx
    ret

; [rsi] Lock
; [rdi] State pointer.
_ZN9Processor12restoreStateER15X64SyscallStatePVm:
    ;; The state pointer is on this thread's kernel stack, so change to it.
    mov     rsp, rdi

    ;; Stack changed, now we can unlock the old thread.
    cmp     rsi, 0
    jz      .no_lock
    ;; Release lock.
    mov     qword [rsi], 1
.no_lock:

;; Restore the registers
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r10
    pop     r9
    pop     r8
    pop     rbp
    pop     rsi
    pop     rdi
    pop     rdx
    pop     rbx
    pop     rax
    pop     r11
    pop     rcx
    pop     rsp

    db 0x48
    sysret

; [rsp+0x8] param3
; [rsp+0x0] return address
; [r9]     param2
; [r8]     param1
; [rcx]    param0
; [rdx]    stack
; [rsi]    address
; [rdi]    Lock
_ZN9Processor10jumpKernelEPVmmmmmmm:
    ;; Load the lock pointer, address and stack to scratch registers.
    mov     r10, rdi
    mov     rax, rsi
    mov     r12, rdx

    ;; Load the parameters into regcall registers.
    mov     rdi, rcx
    mov     rsi, r8
    mov     rdx, r9
    mov     rcx, [rsp+8]

    ;; Change stacks.
    cmp     r12, 0
    jz      .no_stack_change
    mov     rsp, r12
.no_stack_change:

    ;; Stack changed, now we can unlock the old thread.
    cmp     r10, 0
    jz      .no_lock
    ;; Release lock.
    mov     qword [r10], 1
.no_lock:

    ;; Push return address.
    mov     r10, _ZN6Thread12threadExitedEv
    push    r10

    ;; New stack frame.
    xor     rbp, rbp
    ;; Enable interrupts and jump.
    sti
    jmp     rax

; [rsp+0x8] param3
; [rsp+0x0] return address
; [r9]     param2
; [r8]     param1
; [rcx]    param0
; [rdx]    stack
; [rsi]    address
; [rdi]    Lock
_ZN9Processor8jumpUserEPVmmmmmmm:
    ;; Load the lock pointer, address and stack to scratch registers.
    mov     r10, rdi
    mov     rax, rsi
    mov     r12, rdx

    ;; Load the parameters into regcall registers.
    mov     rdi, rcx
    mov     rsi, r8
    mov     rdx, r9
    mov     r15, [rsp+8]    ; Note different register from calling convention: RCX get clobbered!

    ;; Change stacks.
    mov     rsp, r12

    ;; Stack changed, now we can unlock the old thread.
    cmp     r10, 0
    jz      .no_lock
    ;; Release lock.
    mov     qword [r10], 1
.no_lock:

    ;; Push return address.
    mov     r10, _ZN6Thread12threadExitedEv
    push    r10

    ;; New stack frame.
    xor     rbp, rbp
    ;; RFLAGS: interrupts enabled.
    mov     r11, 0x200
    ;; Enable interrupts and jump.
    mov     rcx, rax

    db 0x48
    sysret

_ZN21PerProcessorScheduler28deleteThreadThenRestoreStateEP6ThreadR17X64SchedulerState:
    ; Load the state pointer
    mov rcx, rsi
    
    ; Load the Thread* pointer
    mov rsi, rdi
    
    ; Load new stack and call deleteThread from it - RSI already set from above
    mov rsp, [rcx + 80]
    push rcx
    call _ZN21PerProcessorScheduler12deleteThreadEP6Thread
    pop rcx
    
    ; Restore state
    mov r8, [rcx+0]
    mov r9, [rcx+8]
    mov r10, [rcx+16]
    mov r11, [rcx+24]
    mov r12, [rcx+32]
    mov r13, [rcx+40]
    mov r14, [rcx+48]
    mov r15, [rcx+56]
    mov rbx, [rcx+64]
    mov rbp, [rcx+72]
    mov rsp, [rcx+80]
    mov rdx, [rcx+88]
    
    mov rax, 1
    
    push rdx
    ret

