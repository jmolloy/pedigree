; Copyright (c) 2008-2014, Pedigree Developers
; 
; Please see the CONTRIB file in the root of the source tree for a full
; list of contributors.
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

; void Processor::jumpKernel(volatile uintptr_t *, uintptr_t, uintptr_t,
;                            uintptr_t, uintptr_t, uintptr_t, uintptr_t)
global _ZN9Processor10jumpKernelEPVmmmmmmm
; void PerProcessorScheduler::deleteThreadThenRestoreState(Thread*, SchedulerState&)
global _ZN21PerProcessorScheduler28deleteThreadThenRestoreStateEP6ThreadR20HostedSchedulerState
; syscall entry method (at syscall entry address)
global syscall_enter
; void Processor::restoreState(volatile uintptr_t *, SyscallState &)
global _ZN9Processor12restoreStateER18HostedSyscallStatePVm

; void Thread::threadExited()
extern _ZN6Thread12threadExitedEv
; void PerProcessorScheduler::deleteThread(Thread *)
extern _ZN21PerProcessorScheduler12deleteThreadEP6Thread
; void Processor::restoreState(SchedulerState &, volatile uintptr_t *)
extern _ZN9Processor12restoreStateER20HostedSchedulerStatePVm

; void HostedSyscallManager::syscall(SyscallState &syscallState)
extern _ZN20HostedSyscallManager7syscallER18HostedSyscallState
; Processor::m_ProcessorInformation
extern _ZN9Processor22m_ProcessorInformationE

        
[bits 64]
[section .text]

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
    jmp     rax

_ZN21PerProcessorScheduler28deleteThreadThenRestoreStateEP6ThreadR20HostedSchedulerState:
    ; Load the state pointer
    mov rcx, rsi

    ; Load the Thread* pointer
    mov rsi, rdi

    ; We need to get OFF the current stack as it may get unmapped by the
    ; Thread deletion coming. We will use a temporary stack for the frame.
    ; TODO: this will break if we have multiprocessing.
    mov rsp, safe_stack_top

    ; Ready to go.
    push rcx
    call _ZN21PerProcessorScheduler12deleteThreadEP6Thread
    pop rcx

    ; Get out of here (no need to pass a lock).
    mov rdi, rcx
    xor rsi, rsi
    jmp _ZN9Processor12restoreStateER20HostedSchedulerStatePVm 

[section .syscall exec]
; [rsp+0x10] p5
; [rsp+0x8] p4
; [rsp+0x0] (return address)
; [r9]     p3
; [r8]     p2
; [rcx]    p1
; [rdx]    Error*
; [rsi]    Function
; [rdi]    Service
syscall_enter:
    ; We need to save stack-based registers as we might switch stack.
    mov r10, [rsp + 8]  ; p4
    mov r11, [rsp + 16] ; p5

    ; Preserve current stack.
    push r12
    mov r12, rsp

    ; Switch stacks (MUST be a kernel stack).
    ; Processor::m_Information + 0x158 = kernel stack
    mov rax, _ZN9Processor22m_ProcessorInformationE
    add rax, 0x158
    mov rsp, [rax]

    ; Create the SyscallState
    sub rsp, 8  ; align to 16-byte boundary
    push r12
    push 0    ; result (return value)
    push rdx  ; error pointer
    push 0    ; error (to write into [rdx])
    push r11
    push r10
    push r9
    push r8
    push rcx
    push rsi
    push rdi
    mov rdi, rsp
    call _ZN20HostedSyscallManager7syscallER18HostedSyscallState
    pop rdi
    pop rsi
    pop rcx
    pop r8
    pop r9
    pop r10
    pop r11

    ; error value (modified by syscall)
    pop rax
    pop rdx
    mov [rdx], rax

    ; return value from syscall
    pop rax

    ; Bring back the old stack.
    pop rsp
    pop r12
    ret

; [rsi] Lock
; [rdi] State pointer.
_ZN9Processor12restoreStateER18HostedSyscallStatePVm:
    ; The state pointer is on the current thread kernel stack, so change to it.
    mov     rsp, rdi

    ; Stack changed, now we can unlock the old thread.
    cmp     rsi, 0
    jz      .no_lock
    ; Release lock.
    mov     qword [rsi], 1
.no_lock:

    pop rdi
    pop rsi
    pop rcx
    pop r8
    pop r9
    pop r10
    pop r11

    ; error value
    pop rax
    pop rdx
    mov [rdx], rax

    ; return value from syscall
    pop rax

    ; Bring back the old stack.
    pop rsp
    pop r12
    ret

[section .bss]
safe_stack:
    resb 0x1000
safe_stack_top:
