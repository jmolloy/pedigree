# Copyright (c) 2008-2014, Pedigree Developers
# 
# Please see the CONTRIB file in the root of the source tree for a full
# list of contributors.
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

# ARM vector table, starting at physical address 0

.extern arm_reset_handler
.extern arm_instundef_handler
.extern arm_swint_handler
.extern arm_irq_handler
.extern arm_fiq_handler
.extern arm_prefetch_abort_handler
.extern arm_data_abort_handler
.extern arm_addrexcept_handler

.section .ivt

.global __arm_vector_table
.global __end_arm_vector_table
__arm_vector_table:
# 0x00 - RESET
    __armvec_reset:
        ldr pc,=arm_reset_handler
# 0x04 - UNDEFINED INSTRUCTION
    __armvec_undefinst:
        ldr pc,=arm_instundef_handler
# 0x08 - SUPERVISOR CALL
    __armvec_swint:
        ldr pc,=arm_asm_swint_handler
# 0x0C - PREFETCH ABORT
    __armvec_prefetchabort:
        ldr pc,=arm_asm_prefetch_abort_handler
# 0x10 - DATA ABORT
    __armvec_dataabort:
        ldr pc,=arm_asm_data_abort
# 0x14 - NOT USED
        ldr pc,=arm_addrexcept_handler
# 0x18 - IRQ (interrupt)
    __armvec_irq:
        ldr pc,=arm_asm_irq_handler
# 0x1C - FIQ (fast interrupt)
    __armvec_fiq:
        ldr pc,=arm_fiq_handler
__end_arm_vector_table:

.section .text

##

# Align stack to 8 bytes, preserving old stack in given register.
.macro stack_align reg
    mov \reg, sp
    tst sp, #4
    subeq sp, #4
    push {\reg}
.endm

# Undo stack_align
.macro stack_unwind reg
    pop {\reg}
    mov sp, \reg
.endm

# Generic exception/interrupt handling.
.macro __arm_interrupt entrypoint lr_sub
    # SPSR and R14 can be saved in one operation, so we preserve current SPSR
    # and the return address before switching to SVC mode.
    sub lr, #\lr_sub
    srsdb #0x13!

    # Disable IRQs and switch to SVC mode.
    cpsid i, #0x13

    # Save registers we care about.
    push {r0-r12, lr}

    # Save user stack, lr
    sub sp, #8
    stmia sp, {r13, r14}^

    # Align stack, present InterruptState in r0
    stack_align r0
    bl \entrypoint
    stack_unwind r0

    # Restore user stack, lr
    ldmia sp, {r13, r14}^
    add sp, #8

    # Restore saved registers.
    pop {r0-r12, r14}

    # Inverse of srsdb above - return to caller.
    rfeia sp!
.endm

##

arm_asm_data_abort:
    __ARM_INTERRUPT arm_data_abort_handler 8

arm_asm_irq_handler:
    __ARM_INTERRUPT arm_irq_handler 4

arm_asm_prefetch_abort_handler:
    __ARM_INTERRUPT arm_prefetch_abort_handler 4

arm_asm_swint_handler:
    __ARM_INTERRUPT arm_swint_handler 0
