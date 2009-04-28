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

; void PerProcessorScheduler::contextSwitch(uintptr_t&,uintptr_t&,uintptr_t&)
global _ZN21PerProcessorScheduler13contextSwitchERmS0_RVm
; void PerProcessorScheduler::launchThread(uintptr_t&,uintptr_t&,uintptr_t,uintptr_t,uintptr_t, uintptr_t)
global _ZN21PerProcessorScheduler12launchThreadERmRVmmmmm
; void PerProcessorScheduler::launchThread(uintptr_t&,uintptr_t&,SyscallState&)
global _ZN21PerProcessorScheduler12launchThreadERmRVmR15X64SyscallState
; void Processor::deleteThreadThenContextSwitch(Thread*,uintptr_t&)
global _ZN21PerProcessorScheduler29deleteThreadThenContextSwitchEP6ThreadRm

; void PerProcessorScheduler::deleteThread(Thread*)
extern _ZN21PerProcessorScheduler12deleteThreadEP6Thread
; void Thread::threadExited()
extern _ZN6Thread12threadExitedEv

[bits 64]
[section .text]

; [rdx]    pCurrentThread->lock
; [rsi]    pNextThread->state
; [rdi]    pCurrentThread->state
; [rsp+0]  <Return address>
_ZN21PerProcessorScheduler13contextSwitchERmS0_RVm:
    ; Push the two registers we need to save.
    push   rbp
    push   rbx
    push   r12
    push   r13
    push   r14
    push   r15

    ; Save the stack pointer to pCurrentThread->state.
    mov    [rdi], rsp

    ; Switch stacks.
    mov    rsp, [rsi]

    ; Unlock the current thread.
    mov    qword [rdx], 1

    ; Unsave registers and return.
    pop    r15
    pop    r14
    pop    r13
    pop    r12
    pop    rbx
    pop    rbp
    ret

; [rsi]    pNextThread->state
; [rdi]    pThread
; [rsp+0]  <Return address>
_ZN21PerProcessorScheduler29deleteThreadThenContextSwitchEP6ThreadRm:
    ; Switch stacks.
    mov esp, [rsi]

    ; Save rdi (pThread) first, in case deleteThread mashes it (it's caller-save)
    mov r12, rdi

    ; Call PerProcessorScheduler::deleteThread on the old thread (param already in rdi)
    call _ZN21PerProcessorScheduler12deleteThreadEP6Thread

    ; Now do the actual task switch.
    pop    r15
    pop    r14
    pop    r13
    pop    r12
    pop    rbx
    pop    rbp
    ret

; [r9]     bUsermode
; [r8]     param
; [rcx]    func
; [rdx]    pNextThread->stack
; [rsi]    pCurrentThread->lock
; [rdi]    pCurrentThread->state
; [rsp+0]  <Return address>
_ZN21PerProcessorScheduler12launchThreadERmRVmmmmm:
    cli
    
    ; Create a properly formed stack frame for contextSwitch.
    push   rbp
    push   rbx
    push   r12
    push   r13
    push   r14
    push   r15
    
    ; And set state to be the current stack pointer.
    mov    [rdi], rsp

    ; We don't need the old stack any more - switch.
    mov    rsp, rdx

    ; Unlock the old thread.
    mov    dword [rsi], 1

    ; New stack frame.
    xor    rbp, rbp

    ; Set up the target stack, initially.
    mov    rdi, r8		       ; Parameter in rdi.
    mov    rsi, _ZN6Thread12threadExitedEv
    push   rsi	                       ; Return address on the stack.

    ; Jumping to user mode? go to .jump_to_user_mode.
    cmp    r9, 0
    jne    .jump_to_user_mode

    sti
    jmp    rcx

.jump_to_user_mode:
    ; Syscall return. There's nothing more needed. Stack is set up, and new RIP is in RCX.
    sysret
    
; [rdx]    SyscallState (kernel stack)
; [rsi]    pCurrentThread->lock
; [rdi]    pCurrentThread->state
; [rsp+0]  <Return address>
_ZN21PerProcessorScheduler12launchThreadERmRVmR15X64SyscallState:
    cli

    ; Create a properly formed stack frame for contextSwitch.
    push   rbp
    push   rbx
    push   r12
    push   r13
    push   r14
    push   r15
    
    ; And set state to be the current stack pointer.
    mov    [rdi], rsp

    ; We don't need the old stack any more - switch.
    mov    rsp, rdx

    ; Unlock the old thread.
    mov    dword [rsi], 1

    ; At this point we prepare for the jump to the next task.
    pop    r15
    pop    r14
    pop    r13
    pop    r12
    pop    r10
    pop    r9	
    pop    r8
    pop    rbp
    pop    rsi
    pop    rdi
    pop    rdx
    pop    rbx
    pop    rax
    pop    r11
    pop    rcx
    pop    rsp

    sysret
