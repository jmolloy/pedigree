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

# MIPS32TlbManager::interruptAsm()
.globl _ZN16MIPS32TlbManager12interruptAsmEv
# MIPS32TlbManager::writeTlb(uintptr_t, uintptr_t, uintptr_t)
.globl _ZN16MIPS32TlbManager8writeTlbEjjj
# MIPS32TlbManager::writeTlbWired(uintptr_t, uintptr_t, uintptr_t, uintptr_t)
.globl _ZN16MIPS32TlbManager13writeTlbWiredEjjjj
# MIPS32TlbManager::flush(uintptr_t)
.globl _ZN16MIPS32TlbManager5flushEj
# MIPS32TlbManager::flushAsid(uintptr_t, uintptr_t)
.globl _ZN16MIPS32TlbManager9flushAsidEjj
# MIPS32TlbManager::writeContext(uintptr_t)
.globl _ZN16MIPS32TlbManager12writeContextEj
# MIPS32TlbManager::writeWired(uintptr_t)
.globl _ZN16MIPS32TlbManager10writeWiredEj
# MIPS32TlbManager::writePageMask(uintptr_t)
.globl _ZN16MIPS32TlbManager13writePageMaskEj
# MIPS32TlbManager::getNumEntries()
.globl _ZN16MIPS32TlbManager13getNumEntriesEv

_ZN16MIPS32TlbManager12interruptAsmEv:
  mfc0 $k1, $4      # Get the Context register.
  nop               # Load delay
  lw   $k0, 0($k1)  # Get the TLB entry for the first frame (see TlbManager.h)
  lw   $k1, 8($k1)  # Get the second TLB entry - note the 64-bit gap - that's because this is a 64-bit cpu.
  mtc0 $k0, $2      # Write the EntryLo0 register.
  mtc0 $k1, $3      # Write the EntryLo1 register.
  nop               # Store delay
  tlbwr             # Write EntryLo0/1 in random TLB entry.
  eret              # Exception return.
  nop

_ZN16MIPS32TlbManager8writeTlbEjjj:
  mfc0 $t0, $10     # Save the EntryHi register.
  nop
  mtc0 $a0, $10     # Write the EntryHi register.
  nop
  mtc0 $a1, $2      # Write the EntryLo0 register.
  nop
  mtc0 $a2, $3      # Write the EntryLo1 register.
  nop
  tlbwr             # Random TLB replacement.
  nop
  mtc0 $t0, $10     # Restore the EntryHi register.
  jr $ra            # Return
  nop

_ZN16MIPS32TlbManager13writeTlbWiredEjjjj:
  mfc0 $t0, $10     # Save EntryHi
  mtc0 $a0, $0      # Write the Index register.
  nop
  mtc0 $a1, $10     # Write the EntryHi register.
  nop
  mtc0 $a2, $2      # Write the EntryLo0 register.
  nop
  mtc0 $a3, $3      # Write the EntryLo1 register.
  nop
  tlbwi             # Indexed TLB replacement.
  nop
  mtc0 $t0, $10     # Restore EntryHi.
  nop
  jr $ra            # Return
  nop

_ZN16MIPS32TlbManager5flushEj:
  mfc0 $t0, $10        # Save EntryHi, for its ASID.
  mtc0 $zero, $2       # Destroy EntryLo0
  mtc0 $zero, $3       # Destroy EntryLo1
  li $t1, 0xa0000000   # Load $t1 with an impossible VPN - the base address of KSEG1!

.loop1:
  subu $a0, $a0, 1     # Decrease the given parameter (number of TLB entries) so
                       # that we have a valid index.
  mtc0 $a0, $0         # Write the Index register - select the TLB entry.
  nop                  # Load delay
  mtc0 $t1, $10        # Destroy EntryHi
  addu $t1, $t1, 0x2000 # Increment VPN, so all entries differ.
  tlbwi                # Write the entry back.
  bne $a0, $zero, .loop1 # Keep going until we reached 0.

  nop
  mtc0 $t0, $10        # Restore EntryHi
  jr $ra               # Return
  nop

_ZN16MIPS32TlbManager9flushAsidEjj:
  mfc0 $t0, $10        # Save EntryHi, for its ASID.
  mtc0 $zero, $2       # Destroy EntryLo0
  mtc0 $zero, $3       # Destroy EntryLo1
  li $t1, 0xa0000000   # Load $t1 with an impossible VPN - the base address of KSEG1!

.loop2:
  subu $a0, $a0, 1     # Decrease the given parameter (number of TLB entries) so
                       # that we have a valid index.
  mtc0 $a0, $0         # Write the Index register - select the TLB entry.
  nop                  # Load delay
  tlbr                 # Read TLB Entry.
  nop                  # Load delay.
  mfc0 $t2, $10        # Read EntryHi
  nop                  # Load delay
  and $t2, $t2, 0xFF   # Lower 8 bits of EntryHi are the ASID
  bne $a1, $t2, .keep  # If the ASID != the given ASID, don't destroy it.
  nop                  # Load delay.

  mtc0 $t1, $10        # Destroy EntryHi
  addu $t1, $t1, 0x2000 # Increment VPN, so all entries differ.
  tlbwi                # Write the entry back.

.keep:
  bne $a0, $zero, .loop2 # Keep going until we reached 0.
  nop

  mtc0 $t0, $10        # Restore EntryHi
  jr $ra               # Return
  nop

_ZN16MIPS32TlbManager12writeContextEj:
  mtc0 $a0, $4         # Write parameter to Context.
  jr $ra               # Return.
  nop

_ZN16MIPS32TlbManager10writeWiredEj:
  mtc0 $a0, $6
  jr $ra
  nop

_ZN16MIPS32TlbManager13writePageMaskEj:
  mtc0 $a0, $5
  jr $ra
  nop

.set reorder
.set at

# Trying a new method: TLB probe.
# Accesses to TLB entries appear to be modulo'd by the number of entries, so
# we can't just count on writing sentinels and reading them back until one doesn't
# match (as indices wrap around).
#
# Instead, my algorithm has a sentinel value that increases by 0x2000 every TLB
# write (so that duplicate keys are not stored in the content-addressable memory).
# On every iteration the current TLB entry is read and the lower 12 bits compared
# with the sentinel value. If these match, we have wrapped around and we should 
# exit. If they do not, then this is a new TLB entry.
#
# Please note that after this function the entire TLB will be trashed, as will
# EntryHi, EntryLo0 and EntryLo1.
_ZN16MIPS32TlbManager13getNumEntriesEv:
  li $t0, 0x0        # Index to write
  li $t1, 0x000FFABE # Sentinel - Some processors mask the most sig. 6 bits, and
     	             # all seem to mask the LSB.
  li $t5, 0xABE      # The least sig. 12 bits of the sentinel.
  li $t4, 0x400      # Stop searching if you reach this number.
1:
  # Write Index register.
  mtc0 $t0, $0
  nop

  # Read TLB entry.
  tlbr
  nop

  # Read EntryLo0 value.
  mfc0 $t3, $2
  nop

  # Is this value one that we've previously set?
  and $t3, $t3, 0xFFF
  beq $t3, $t5, 1f            # If yes, finish.

  # This is a new TLB register, so write our sentinel (to EntryLo0)
  mtc0 $t1, $2
  nop

  # Write TLB entry.
  tlbwi
  nop

  # Increment the index counter, and increase the sentinel to avoid duplicate TLB
  # entries.
  addiu $t0, $t0, 1
  addiu $t1, $t1, 0x2000

  # If we've gone too far (something went wrong) exit now.
  beq $t0, $t4, 1f

  # Loop.
  j 1b
  nop

1:
  move $v0, $t0
  jr $ra
  nop
