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
  beq $k1, $zero, .stack_good # If sp in kuseg, skip the stack change.
  nop
  li $k0, 0x80020000     # This will be our kernel stack pointer.
.stack_good:
  # Here, $k0 holds the value of our kernel stack. $sp is still it's pre-exception value.
  # $k1 holds junk.

  # TODO:: push all regs to $sp here.

  # TODO:: Set $fp.
  move $sp, $k0

  # First parameter to function is a pointer to InterruptState (top of stack).
  addu $a0, $sp, $zero   # $a0 (first parameter) = $sp+0.

  # Jump to our C++ code.
  jal _ZN22MIPS32InterruptManager9interruptER20MIPS32InterruptState
  nop # Delay slot.
  # TODO:: Pop all regs.
  
  # Make it so that EPC is in $k1 when we get here.
  jr $k1 # Jump back to the program address.
  rfe    # Return from exception in delay slot.

.set at
.set reorder
