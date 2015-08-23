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
        ldr pc,=arm_swint_handler
# 0x0C - PREFETCH ABORT
    __armvec_prefetchabort:
        ldr pc,=arm_prefetch_abort_handler
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

.macro __arm_interrupt entrypoint
    # Save some registers so we can trash them to get interrupt info.
    stmia r13, {r4-r6}

    # Save this point on the stack so we can restore r4-r5.
    mov r4, r13
    sub r5, lr, #4

    # Retrieve old stack pointer.
    add r6, r4, #12

    # Now that we've preserved registers via the IRQ save area, switch into
    # SVC mode to properly handle the IRQ. Disable IRQs while we're at it.
    msr cpsr_c, #(3<<6 | 0x13)

    # Push return address (lr - 4).
    stmfd sp!, {r5}

    # Push old stack pointer. (nb: this isn't correct yet)
    stmfd sp!, {r6}

    # Restore trashed registers.
    ldmia r4, {r4-r6}

    # Save registers from the IRQ, and the given LR.
    stmfd sp!, {r0-r12, lr}

    # Finally save SPSR.
    mrs r0, spsr
    stmfd sp!, {r0}

    # InterruptState
    mov r0, sp
    bl \entrypoint

    # Restore SPSR
    ldmfd sp!, {r0}
    msr spsr_cxsf, r0

    # Restore to where we were before the interrupt.
    # We need to skip over the saved stack before popping pc.
    ldmfd sp!, {r0-r12, lr}
    add sp, sp, #4
    ldmfd sp!, {pc}
.endm

##

arm_asm_data_abort:
    __ARM_INTERRUPT arm_data_abort_handler

arm_asm_irq_handler:
    __ARM_INTERRUPT arm_irq_handler
