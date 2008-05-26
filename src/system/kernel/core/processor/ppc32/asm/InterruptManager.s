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

# PPC32InterruptManager::interrupt(InterruptState &interruptState)
.extern _ZN21PPC32InterruptManager9interruptER19PPC32InterruptState

# Interrupt handlers
.global isr_reset
.global isr_machine_check
.global isr_dsi
.global isr_isi
.global isr_interrupt
.global isr_alignment
.global isr_program
.global isr_fpu
.global isr_decrementer
.global isr_sc
.global isr_trace
.global isr_perf_mon
.global isr_instr_breakpoint
.global isr_system_management
.global isr_thermal_management

isr_reset:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20, 0    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_machine_check:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20, 1    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_dsi:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20, 2    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_isi:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20, 3    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_interrupt:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20, 4    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_alignment:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20, 5    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_program:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20, 6    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_fpu:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20, 7    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_decrementer:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20, 8    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_sc:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20, 9    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_trace:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20,10    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_perf_mon:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20,11    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_instr_breakpoint:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20,12    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_system_management:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20,13    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

isr_thermal_management:
  mtsprg  0, 20    # Move r20 into sprg0
  mtsprg  1, 21    # Move r21 into sprg1 ... now we have 2 registers to work with.

  li      20,14    # r20 = 0 - exception vector.

  mfmsr   21       # Move MSR into r21
  ori    21, 21,0x30 # Enable instruction translation
  mtmsr   21       # Write MSR back

  ba    isr_common

# We are jumped to here with r20 = interrupt vector, r21 = junk, but usable,
# no other registers usable, instruction translation enabled.
# SPRG0 = saved r20.
# SPRG1 = saved r21
# SPRG3 = kernel stack.
isr_common:
  mfcr   21            # Get the condition register
  mtsprg 2, 21         # Save CR in SPRG2 - we're about to do a conditional.

  mfsprg 21, 3         # r21 = kernel stack.

  cmpi   0, 21, 0      # Compare the values of register 21 and $0, setting condition field 0
#  bne    1f            # If r21 != 0, go to 1:

  mr     21, 1         # The kernel stack was 0, use the current stack as the stack pointer

1:
  stw    20, -0xa0(21) # Save the interrupt number
  mfctr  20            # Get CTR
  stw    20, -0x9c(21) # Save CTR
  mflr   20            # Get LR
  stw    20, -0x98(21) # Save LR
  mfsprg 20, 2         # Get CR back
  stw    20, -0x94(21) # Save CR
  mfspr  20, 26        # Get SRR0 in r20
  stw    20, -0x90(21) # Save SRR0
  mfspr  20, 27        # Get SRR1 in r20
  stw    20, -0x8c(21) # Save SRR1
  mfspr  20, 18        # Get DSISR
  stw    20, -0x88(21) # Save DSISR
  mfspr  20, 19        # Get DAR
  stw    20, -0x84(21) # Save DAR
  stw    0 , -0x80(21) # r0
  stw    1 , -0x7c(21)
  stw    2 , -0x78(21)
  stw    3 , -0x74(21)
  stw    4 , -0x70(21)
  stw    5 , -0x6c(21)
  stw    6 , -0x68(21)
  stw    7 , -0x64(21)
  stw    8 , -0x60(21)
  stw    9 , -0x5c(21)
  stw    10, -0x58(21)
  stw    11, -0x55(21)
  stw    12, -0x50(21)
  stw    13, -0x4c(21)
  stw    14, -0x48(21)
  stw    15, -0x44(21)
  stw    16, -0x40(21)
  stw    17, -0x3c(21)
  stw    18, -0x38(21)
  stw    19, -0x34(21)
  mfsprg 20, 0         # Retrieve r20's value
  stw    20, -0x30(21)
  mfsprg 20, 1         # Retrieve r21's value
  stw    20, -0x2c(21)
  stw    22, -0x28(21)
  stw    23, -0x24(21)
  stw    24, -0x20(21)
  stw    25, -0x1c(21)
  stw    26, -0x18(21)
  stw    27, -0x14(21)
  stw    28, -0x10(21)
  stw    29, -0x0c(21)
  stw    30, -0x08(21)
  stw    31, -0x04(21)

  addi   1, 21, -0xa0  # Set the stack pointer

  mr     3, 1          # First argument is a pointer to the stack.
  bl     _ZN21PPC32InterruptManager9interruptER19PPC32InterruptState
