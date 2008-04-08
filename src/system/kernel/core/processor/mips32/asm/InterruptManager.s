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

.set noreorder
.set noat

# MIPS32InterruptManager::interrupt(InterruptState &interruptState)
.extern _ZN22MIPS32InterruptManager9interruptER20MIPS32InterruptState

.globl mips32_exception
mips32_exception:
nop
.bleh:
  # If the stack pointer is in kseg0, we assume we were interrupted in the kernel, and
  # that the stack is valid.
  li $k0, 0x80000000     # $k0 is the start of kseg0
  sltu $k1, $sp, $k0     # if $sp < kseg0, $k1 == 1. Else $k1 == 0.
  move $k0, $sp          # $k0 is the current stack pointer.
  beq $k1, $zero, .stack_good # If sp in kseg0, skip the stack change.
  nop
li $k0, 0x80020000     # This will be our kernel stack pointer.
.stack_good:
  # Here, $k0 holds the value of our kernel stack. $sp is still its pre-exception value.
  # $k1 holds junk.
  addiu $k0, $k0, -132     # Reserve some space on the stack.
  sw $at, 0($k0)
  sw $v0, 4($k0)
  sw $v1, 8($k0)
  sw $a0, 12($k0)
  sw $a1, 16($k0)
  sw $a2, 20($k0)
  sw $a3, 24($k0)
  sw $t0, 28($k0)
  sw $t1, 32($k0)
  sw $t2, 36($k0)
  sw $t3, 40($k0)
  sw $t4, 44($k0)
  sw $t5, 48($k0)
  sw $t6, 52($k0)
  sw $t7, 56($k0)
  sw $s0, 60($k0)
  sw $s1, 64($k0)
  sw $s2, 68($k0)
  sw $s3, 72($k0)
  sw $s4, 76($k0)
  sw $s5, 80($k0)
  sw $s6, 84($k0)
  sw $s7, 88($k0)
  sw $t8, 92($k0)
  sw $t9, 96($k0)
  sw $gp, 100($k0)
  sw $sp, 104($k0)
  sw $fp, 108($k0)
  sw $ra, 112($k0)
  mfc0 $k1, $12        # SR
  nop
  sw $k1, 116($k0)
  mfc0 $k1, $13        # Cause
  nop
  sw $k1, 120($k0)
  mfc0 $k1, $14        # EPC
  nop
  sw $k1, 124($k0)
  mfc0 $k1, $8         # BadVaddr
  sw $k1, 128($k0)

  move $sp, $k0        # Set stack pointer to be the bottom limit of our stack frame.
  addiu $fp, $sp, 132  # Set the frame pointer to be the stack pointer + 116.

  addiu $sp, -20       # Caller needs to reserve at least 16 bytes on the stack, and must be
                       # 8-byte aligned, so we reserve 20.

  # First parameter to function is a pointer to InterruptState (top of stack).
  move $a0, $k0        # $a0 (first parameter) = $k0.
  move $s0, $k0        # Save $k0 in a temporary.

  # Jump to our C++ code.
  jal _ZN22MIPS32InterruptManager9interruptER20MIPS32InterruptState
  nop                  # Delay slot.
  
  move $k0, $s0        # $k0 is now restored to what it was before.
  
  lw $k1, 124($k0)     # EPC
  mtc0 $k1, $14
  nop
  
  lw $at, 0($k0)
  lw $v0, 4($k0)
  lw $v1, 8($k0)
  lw $a0, 12($k0)
  lw $a1, 16($k0)
  lw $a2, 20($k0)
  lw $a3, 24($k0)
  lw $t0, 28($k0)
  lw $t1, 32($k0)
  lw $t2, 36($k0)
  lw $t3, 40($k0)
  lw $t4, 44($k0)
  lw $t5, 48($k0)
  lw $t6, 52($k0)
  lw $t7, 56($k0)
  lw $s0, 60($k0)
  lw $s1, 64($k0)
  lw $s2, 68($k0)
  lw $s3, 72($k0)
  lw $s4, 76($k0)
  lw $s5, 80($k0)
  lw $s6, 84($k0)
  lw $s7, 88($k0)
  lw $t8, 92($k0)
  lw $t9, 96($k0)
  lw $gp, 100($k0)
  lw $sp, 104($k0)
  lw $fp, 108($k0)
  lw $ra, 112($k0)

  eret  # R4000 only!

.set at
.set reorder
