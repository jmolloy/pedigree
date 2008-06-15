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
# extern "C" void sdr1_trampoline(uint32_t sdr1)
.global sdr1_trampoline

start:
  lis 1, stack@ha
  addi 1,1, stack@l
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
  nop

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
