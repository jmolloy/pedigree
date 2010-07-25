#
# Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

# uintptr_t Processor::getBasePointer()
.global _ZN9Processor14getBasePointerEv
# uintptr_t Processor::getStackPointer()
.global _ZN9Processor15getStackPointerEv
# uintptr_t Processor::getInstructionPointer()
.global _ZN9Processor21getInstructionPointerEv
# uintptr_t Processor::getDebugStatus()
.global _ZN9Processor14getDebugStatusEv

_ZN9Processor14getBasePointerEv:
  mov r0,fp
  mov pc,lr

_ZN9Processor15getStackPointerEv:
  mov r0,sp
  mov pc,lr

_ZN9Processor21getInstructionPointerEv:
  mov r0,pc
  mov pc,lr

# bool Processor::saveState(SchedulerState &)
.global _ZN9Processor9saveStateER19ARMV7SchedulerState
# void Processor::restoreState(SchedulerState &, volatile uintptr_t *)
.global _ZN9Processor12restoreStateER19ARMV7SchedulerStatePVm
# void Processor::restoreState(volatile uintptr_t *, SyscallState &)
.global _ZN9Processor12restoreStateER19ARMV7InterruptStatePVm
# void Processor::jumpKernel(volatile uintptr_t *, uintptr_t, uintptr_t,
#                            uintptr_t, uintptr_t, uintptr_t, uintptr_t)
.global _ZN9Processor10jumpKernelEPVmmmmmmm
# void Processor::jumpUser(volatile uintptr_t *, uintptr_t, uintptr_t,
#                          uintptr_t, uintptr_t, uintptr_t, uintptr_t)
.global _ZN9Processor8jumpUserEPVmmmmmmm
# void PerProcessorScheduler::deleteThreadThenRestoreState(Thread*, SchedulerState&)
.global _ZN21PerProcessorScheduler28deleteThreadThenRestoreStateEP6ThreadR19ARMV7SchedulerState

# void Thread::threadExited()
.extern _ZN6Thread12threadExitedEv
# void PerProcessorScheduler::deleteThread(Thread *)
.extern _ZN21PerProcessorScheduler12deleteThreadEP6Thread

_ZN9Processor9saveStateER19ARMV7SchedulerState:
    # Store state
    mov r12, lr
    stmia r0, {r4 - r11, r12, sp}^
    # , r13, r14}^

    mov r0, #0
    bx lr

_ZN9Processor12restoreStateER19ARMV7SchedulerStatePVm:
    # Load state
    ldmia r0, {r4 - r11, r12, sp}^
    # , r13, r14}^
    mov lr, r12
    
    # mov sp, r0
    
    # TODO: Save r4
    
    # Check for a passed lock, unlock if it exists
    cmp r1, #0
    beq .no_lock
    mov r4, #1
    str r4, [r1]
.no_lock:
    
    # Restore old stack
    # add sp, r0, #(11 * 4)
    
    bx lr

_ZN9Processor12restoreStateER19ARMV7InterruptStatePVm:
    # Load SP (== InterruptState location)
    str sp, [r0]
    
    # TODO: Save r4
    
    # Check for a passed lock, unlock if it exists
    cmp r1, #0
    beq .no_lock1
    mov r4, #1
    str r4, [r1]
.no_lock1:

    # Done.
    # TODO: Not done, gotta restore at least LR to get back to the right place
    bx lr

# void Processor::jumpKernel(volatile uintptr_t *, uintptr_t, uintptr_t,
#                            uintptr_t, uintptr_t, uintptr_t, uintptr_t)
_ZN9Processor10jumpKernelEPVmmmmmmm:
    
    # Load the lock pointer
    mov r8, r0
    
    # Save the jump destination for future reference
    mov r9, r1
    
    # Note: we don't save all the registers we smash here because we're never
    # returning from this function to the caller.
    
    # Load parameters
    mov r4, r3
    ldr r5, [sp]
    ldr r6, [sp, #4]
    ldr r7, [sp, #8]
    
    # Load the new stack pointer
    cmp r2, #0
    movne sp, r2
    
    # Check for a passed lock, unlock if it exists
    cmp r8, #0
    beq .no_lock2
    mov r11, #1
    str r11, [r8]
.no_lock2:

    # Enable interrupts
    mrs r0, cpsr
    orr r0, r0, #0x80
    msr cpsr_c, r0

    # Load parameters for the call
    mov r0, r4
    mov r1, r5
    mov r2, r6
    mov r3, r7
    
    # Jump to the address - never to return
    ldr lr, =_ZN6Thread12threadExitedEv
    bx r9

_ZN9Processor8jumpUserEPVmmmmmmm:
    bx lr

_ZN21PerProcessorScheduler28deleteThreadThenRestoreStateEP6ThreadR19ARMV7SchedulerState:

    # r0 = Thread to delete
    # r1 = New thread state

    # Load state, including stack pointer
    ldmia r1, {r4 - r11, r12, sp}^
    mov lr, r12
   
    # Delete the thread completely (r0 preserved above)
    bl _ZN21PerProcessorScheduler12deleteThreadEP6Thread
    
    # Complete the restore
    bx lr
    
.section .init.text

.global init_stacks
init_stacks:

    # Read the CPSR, mask out the lower 6 bits, and enter Abort mode
    mrs r0, cpsr
    bic r0, r0, #0x3F
    orr r1, r0, #0x17
    msr cpsr_c, r1
    
    ldr sp, =stack_abort+0x10000

    # Read the CPSR, mask out the lower 6 bits, and enter FIQ mode
    mrs r0, cpsr
    bic r0, r0, #0x3F
    orr r1, r0, #0x11
    msr cpsr_c, r1
    
    ldr sp, =stack_fiq+0x10000
    
    # Read the CPSR, mask out the lower 6 bits, and enter SVC mode
    mrs r0, cpsr
    bic r0, r0, #0x3F
    orr r1, r0, #0x13
    msr cpsr_c, r1
    
    ldr sp, =stack_svc+0x20000
    
    bx lr

.section .bss
.comm stack_svc, 0x20000
.comm stack_irq, 0x10000
.comm stack_fiq, 0x10000
.comm stack_abort, 0x10000
.comm irq_save, 0x10
