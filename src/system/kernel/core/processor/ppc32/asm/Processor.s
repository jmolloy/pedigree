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

# start()
.global start
# uintptr_t Processor::getBasePointer()
.global _ZN9Processor14getBasePointerEv
# uintptr_t Processor::getStackPointer()
.global _ZN9Processor15getStackPointerEv
# uintptr_t Processor::getInstructionPointer()
.global _ZN9Processor21getInstructionPointerEv
# uintptr_t Processor::getDebugStatus()
.global _ZN9Processor14getDebugStatusEv
# bool Processor::getInterrupts()
.global _ZN9Processor13getInterruptsEv
# void Processor::contextSwitch(PPC32InterruptState*)
.global _ZN9Processor13contextSwitchEP19PPC32InterruptState
# extern "C" void sdr1_trampoline(uint32_t sdr1)
.global sdr1_trampoline

start:
  lis 1, stack@ha
  addi 1,1, stack@l
  addi 1,1, 32764
  addi 1,1, 32764
#  li 2, 0x0
#  mtlr 2
  b _main

_ZN9Processor14getBasePointerEv:
  nop

_ZN9Processor15getStackPointerEv:
  nop

_ZN9Processor21getInstructionPointerEv:
  nop

_ZN9Processor13getInterruptsEv:
  mfmsr 3           # Grab the MSR in r3
  andi. 3, 3, 0x8000 # AND with MSR_EE
  blr

_ZN9Processor13contextSwitchEP19PPC32InterruptState:
  mr    21, 3          # R21 = bottom of interrupt state (lowest address)
  addi  21, 21, 0xa4   # R21 = top of interrupt state (highest address)
  lwz     31, -0x04(21)
  lwz     30, -0x08(21)
  lwz     29, -0x0c(21)
  lwz     28, -0x10(21)
  lwz     27, -0x14(21)
  lwz     26, -0x18(21)
  lwz     25, -0x1c(21)
  lwz     24, -0x20(21)
  lwz     23, -0x24(21)
  lwz     22, -0x28(21)
  lwz     20, -0x2c(21)
#  mtsprg   1, 20         # Save r21's value again
  lwz     20, -0x30(21)
#  mtsprg   0, 20         # Save r20's value again
  lwz     19, -0x34(21)
  lwz     18, -0x38(21)
  lwz     17, -0x3c(21)
  lwz     16, -0x40(21)
  lwz     15, -0x44(21)
  lwz     14, -0x48(21)
  lwz     13, -0x4c(21)
  lwz     12, -0x50(21)
  lwz     11, -0x54(21)
  lwz     10, -0x58(21)
  lwz      9, -0x5c(21)
  lwz      8, -0x60(21)
  lwz      7, -0x64(21)
  lwz      6, -0x68(21)
  lwz      5, -0x6c(21)
  lwz      4, -0x70(21)
  lwz      3, -0x74(21)
  lwz      2, -0x78(21)
  lwz      1, -0x7c(21)
  lwz      0, -0x80(21)
#   lwz     20, -0x84(21) # Get DAR
#   mtspr  19, 20        # Save DAR
#   lwz     20, -0x88(21) # Get DSISR
#   mtspr  18, 20        # Save DSISR
  lwz     20, -0x8c(21) # Get SRR1
  mtspr  27, 20        # Save SRR1
  lwz     20, -0x90(21) # Get SRR0
  mtspr  26, 20        # Save SRR0
  lwz     20, -0x94(21) # Get CR
  mtcr   20            # Save CR
  lwz     20, -0x98(21) # Get LR
  mtlr   20            # Save LR
  lwz     20, -0x9c(21) # Get CTR
  mtctr  20            # Save CTR
  lwz     20, -0xa0(21) # Get XER
  mtxer  20            # Save XER

  lwz     20, -0x30(21) # Get r20's value back
  lwz     21, -0x2c(21) # And r21.

  rfi                  # Return from interrupt


.section .sdr1_trampoline, "a"
sdr1_trampoline:
  sync
  isync
  lis 4, 0x0000       # r4 = 0x60000000
  mtsr 0, 4
  addi 4, 4, 1        # r4 = 0x60000001
  mtsr 1, 4
  addi 4, 4, 1
  mtsr 2, 4
  addi 4, 4, 1
  mtsr 3, 4
  addi 4, 4, 1
  mtsr 4, 4
  addi 4, 4, 1
  mtsr 5, 4
  addi 4, 4, 1
  mtsr 6, 4
  addi 4, 4, 1
  mtsr 7, 4
  addi 4, 4, 1
  mtsr 8, 4
  addi 4, 4, 1
  mtsr 9, 4
  addi 4, 4, 1
  mtsr 10, 4
  addi 4, 4, 1
  mtsr 11, 4
  addi 4, 4, 1
  mtsr 12, 4
  addi 4, 4, 1
# mtsr 13, 4
  addi 4, 4, 1
  mtsr 14, 4
  addi 4, 4, 1
  mtsr 15, 4
  sync
  isync
  mtsdr1 3   # Parameter -> sdr1
  sync
  isync
  lis 4, 0xFFFF
  ori 4, 4, 0xF000
1:
  tlbie 4
  addi 4, 4, -0x1000
  cmp 7, 4, 0x0
  bne 1b
  sync
  isync
  blr        # Jump back.

.section .bss
stack:
  .rept 32768
  .byte 0
  .endr
  .rept 32768
  .byte 0
  .endr

