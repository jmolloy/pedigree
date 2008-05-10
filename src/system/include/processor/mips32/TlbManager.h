/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef MIPS32_TLBMANAGER_H
#define MIPS32_TLBMANAGER_H

#include <processor/types.h>
#include <processor/state.h>
#include <processor/InterruptHandler.h>

/** A class for manipulating and refilling the translation lookaside buffer. */
class MIPS32TlbManager : public InterruptHandler
{
public:
  /** Retrieves the singleton instance of this Tlb manager. */
  static MIPS32TlbManager &instance();

  /** Initialises the TLB */
  void initialise();

  /** The assembler code for handling a TLB refill.
   * \note Defined in asm/Tlb.s */
  static void interruptAsm();

  /** Writes a random TLB entry
   * \note Defined in asm/Tlb.s */
  static void writeTlb(uintptr_t entryHi, uintptr_t entryLo0, uintptr_t entryLo1);
  /** Writes a specific TLB entry
   * \note Defined in asm/Tlb.s */
  static void writeTlbWired(uintptr_t tlbEntry, uintptr_t entryHi, uintptr_t entryLo0, uintptr_t entryLo1);
  /** Flushes the entire TLB
   * \note Defined in asm/Tlb.s */
  static void flush(uintptr_t numEntries);
  /** Flushes the TLB of any entries with the given ASID.
   * \note Defined in asm/Tlb.s */
  static void flushAsid(uintptr_t numEntries, uintptr_t asid);
  /** Writes the given value into the Context register.
   * \note Defined in asm/Tlb.s */
  static void writeContext(uintptr_t context);
  /** Writes the given value into the Wired register.
   * \note Defined in asm/Tlb.s */
  static void writeWired(uintptr_t wired);
  /** Writes the given value into the PageMask register.
   * \note Defined in asm/Tlb.s */
  static void writePageMask(uintptr_t pageMask);
  /** Returns the number of TLB entries.
   * \note Defined in asm/Tlb.s */
  static uintptr_t getNumEntries();

  //
  // InterruptHandler interface.
  //
  void interrupt(size_t interruptNumber, InterruptState &state);

private:
  /** Default constructor. */
  MIPS32TlbManager();
  /** Destructor. */
  ~MIPS32TlbManager();

  /** The singleton instance of this class. */
  static MIPS32TlbManager m_Instance;

  /** The number of entries in the TLB. */
  uint32_t m_nEntries;
};

#endif
