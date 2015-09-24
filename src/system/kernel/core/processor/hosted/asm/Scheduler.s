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

; void PerProcessorScheduler::deleteThreadThenRestoreState(Thread*, SchedulerState&)
global _ZN21PerProcessorScheduler28deleteThreadThenRestoreStateEP6ThreadR20HostedSchedulerState
; syscall entry method (at syscall entry address)
global syscall_enter
; void Processor::restoreState(volatile uintptr_t *, SyscallState &)
global _ZN9Processor12restoreStateER18HostedSyscallStatePVm

; void PerProcessorScheduler::deleteThread(Thread *)
extern _ZN21PerProcessorScheduler12deleteThreadEP6Thread
; void Processor::restoreState(SchedulerState &, volatile uintptr_t *)
extern _ZN9Processor12restoreStateER20HostedSchedulerStatePVm

; void HostedSyscallManager::syscall(SyscallState &syscallState)
extern _ZN20HostedSyscallManager7syscallER18HostedSyscallState
; Processor::m_ProcessorInformation
extern _ZN9Processor22m_ProcessorInformationE

; void *safe_stack_top
global safe_stack_top

[bits 64]
[section .text]

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

    jmp syscall_tail

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

    ; Preserve callee-save registers, and the current stack.
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rsp

    ; Switch stacks (MUST be a kernel stack).
    ; Processor::m_Information + 0x158 = kernel stack
    mov rax, _ZN9Processor22m_ProcessorInformationE
    add rax, 0x158
    mov rsp, [rax]

    ; Create the SyscallState
    sub rsp, 8 ; align to 16-byte boundary
    push r12
    push 0     ; result (return value)
    push rdx   ; error pointer
    push 0     ; error (to write into [rdx])
    push r11
    push r10
    push r9
    push r8
    push rcx
    push rsi
    push rdi
    mov rdi, rsp
    call _ZN20HostedSyscallManager7syscallER18HostedSyscallState
syscall_tail:
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
    cmp rdx, 0
    je .noerror
    mov [rdx], rax
.noerror:

    ; return value from syscall
    pop rax

    ; Bring back the old stack and callee-save registers.
    pop rsp
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

[section .bss]
safe_stack:
    resb 0x1000
safe_stack_top:
