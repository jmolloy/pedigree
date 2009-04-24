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
global _ZN21PerProcessorScheduler12launchThreadERmRVmR17X86InterruptState
; void Processor::deleteThreadThenContextSwitch(Thread*,uintptr_t&)
global _ZN21PerProcessorScheduler29deleteThreadThenContextSwitchEP6ThreadRm

; void PerProcessorScheduler::deleteThread(Thread*)
extern _ZN21PerProcessorScheduler12deleteThreadEP6Thread
; void Thread::threadExited()
extern _ZN6Thread12threadExitedEv

[bits 32]
[section .text]

; [esp+12] pCurrentThread->lock
; [esp+8] pNextThread->state
; [esp+4]  pCurrentThread->state
; [esp+0]  <Return address>
_ZN21PerProcessorScheduler13contextSwitchERmS0_RVm:
    ; Push the two registers we need to save.
    push ebp
    mov ebp, esp
    push ebx

    ; Save the stack pointer to pCurrentThread->state.
    mov eax, [ebp+8]
    mov [eax], esp

    ; Switch stacks.
    mov eax, [ebp+12]
    mov esp, [eax]

    ; Unlock the current thread.
    mov dword [ebp+16], 0

    ; Unsave registers and return.
    pop ebx
    pop ebp
    ret

; [esp+12] pNextThread->state
; [esp+8]  pThread
; [esp+4]  <Return address>
_ZN21PerProcessorScheduler29deleteThreadThenContextSwitchEP6ThreadRm:
    ; Save pThread.
    mov eax, [esp+4]

    ; Switch stacks.
    mov ecx, [esp+8]
    mov esp, [ecx]

    ; Call PerProcessorScheduler::deleteThread on the old thread.
    push eax
    call _ZN21PerProcessorScheduler12deleteThreadEP6Thread
    pop eax

    ; Now do the actual task switch.
    pop ebx
    pop ebp
    ret

; [esp+28] bUsermode
; [esp+24] param
; [esp+20] func
; [esp+16] pNextThread->stack
; [esp+12] pCurrentThread->lock
; [esp+8]  pCurrentThread->state
; [esp+4]  <Return address>
_ZN21PerProcessorScheduler12launchThreadERmRVmmmmm:
    ; Pop the return address, state, lock, stack and func and save them.
    pop    eax          ; return address
    pop    ecx          ; state
    pop    edx          ; lock
    pop    esi          ; stack
    pop    edi          ; func

    ; Push the return address back onto the stack.
    push   eax

    ; Now the stack contains a return address plus two parameters, which is what
    ; contextSwitch expects. Add in EBP and EBX to make a proper contextSwitch stack
    ; frame.
    push   ebp
    push   ebx

    ; And set state to be the current stack pointer.
    mov    [ecx], esp

    ; Move the remaining two parameters into registers.
    mov    eax, [esp+12] ; param
    mov    ecx, [esp+16] ; usermode

    ; We don't need the old stack any more - switch.
    mov    esp, esi

    ; Unlock the old thread.
    mov    dword [edx], 0

    ; At this point we prepare for the jump to the next task. Register contents:
    ;   eax: param
    ;   ecx: usermode
    ;   edi: func

    ; Jumping to user mode? go to .jump_to_user_mode.
    cmp    ecx, 0
    jne    .jump_to_user_mode

    ; New stack frame.
    xor    ebp, ebp

    ; Jumping to kernel mode - push the parameter and return address then jmp.
    push   eax
    push   _ZN6Thread12threadExitedEv
    sti
    jmp    edi

.jump_to_user_mode:
    ; If we're jumping to user mode, we need to set up an interrupt state.

    ; Set up the target stack, initially.
    push   eax
    push   _ZN6Thread12threadExitedEv

    ; Load all segment descriptors with the user-mode selector.
    mov    ax, 0x23
    mov    ds, ax
    mov    es, ax
    mov    fs, ax
    mov    gs, ax

    ; Save a copy of the stack pointer in its current position - this will
    ; become the target stack.
    mov    eax, esp
    ; Stack segment (SS)
    push   0x23
    ; Stack pointer
    push   eax

    ; EFLAGS: Push to stack, pop, OR in the IF flag (interrupt enable, 0x200) and
    ; push again.
    pushfd
    pop    eax
    or     eax, 0x200
    push   eax

    ; Code segment (CS)
    push   0x1B
    ; EIP
    push   edi

    ; Interrupt-return to the start function.
    iret

; [esp+12] SyscallState (kernel stack)
; [esp+8] pCurrentThread->lock
; [esp+4]  pCurrentThread->state
; [esp+0]  <Return address>
_ZN21PerProcessorScheduler12launchThreadERmRVmR17X86InterruptState:
    ; Pop the return address, state, lock, and stack and save them.
    pop    eax          ; return address
    pop    ecx          ; state
    pop    edx          ; lock
    pop    esi          ; kernel stack / SyscallState

    push dword 0
    push dword 0

    ; Push the return address back onto the stack.
    push   eax

    ; Now the stack contains a return address plus two parameters, which is what
    ; contextSwitch expects. Add in EBP and EBX to make a proper contextSwitch stack
    ; frame.
    push   ebp
    push   ebx

    ; And set state to be the current stack pointer.
    mov    [ecx], esp

    ; We don't need the old stack any more - switch.
    mov    esp, [esi]

    ; Unlock the old thread.
    mov    dword [edx], 0

    ; At this point we prepare for the jump to the next task.

    ; Restore the registers
    pop ds
    mov ax, ds
    mov es, ax
    popa

    ; Remove the errorcode and the interrupt number from the stack
    add esp, 0x08
    iret
