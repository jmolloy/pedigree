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

#ifndef KERNEL_PROCESSOR_PPC32_HASHEDPAGETABLE_H
#define KERNEL_PROCESSOR_PPC32_HASHEDPAGETABLE_H

#include <processor/types.h>
#include "Translation.h"

#define HTAB_VIRTUAL 0xC0100000

/**
 * The PPC hashed page table that is used almost as a secondary PTE cache by the processor.
 * It is a finite size, so not all PTEs may be able to fit in it (hence why it is used as a
 * secondary cache of sorts.
 *
 * The size is set at initialisation time, and is based on the amount of available RAM.
 */
class HashedPageTable
{
public:
  HashedPageTable();
  ~HashedPageTable();

  /** Obtains the singleton instance of the HashedPageTable. */
  static HashedPageTable &instance();

  /** Initialises the page table. */
  void initialise(Translations &translations, uint32_t ramMax);

  /** Adds a page table entry mapping effectiveAddress to physicalAddress for the given VSID, with
      mode 'mode'.
      \param effectiveAddress The EA (virtual address) to map.
      \param physicalAddress The physical address to map to.
      \param mode The mode of the mapping - in VirtualAddressSpace form (VirtualAddressSpace::WriteThrough etc)
      \param vsid The virtual space identifier. */
  void addMapping(uint32_t effectiveAddress, uint32_t physicalAddress, uint32_t mode, uint32_t vsid);

  /** Removes a mapping for the given effective address, in the given VSID. */
  void removeMapping(uint32_t effectiveAddress, uint32_t vsid);

  /** Returns true if a mapping exists for effectiveAddress in the given VSID. */
  bool isMapped(uint32_t effectiveAddress, uint32_t vsid);

  /** Returns the mapping for effectiveAddress in VSID. If there is no such mapping, the return value is undefined. */
  uint32_t getMapping(uint32_t effectiveAddress, uint32_t vsid);

private:
  HashedPageTable(const HashedPageTable &);
  HashedPageTable &operator = (const HashedPageTable &);

  /** A page table entry (PTE) */
  struct PTE
  {
    uint32_t v : 1;     // Entry valid?
    uint32_t vsid : 24; // Virtual segment ID
    uint32_t h : 1;     // Hash function identifier
    uint32_t api : 6;   // Abbreviated page index.
    uint32_t rpn : 20;  // Physical page number.
    uint32_t reserved1 : 3;
    uint32_t r : 1;     // Reference bit.
    uint32_t c : 1;     // Change bit.
    uint32_t w : 1;   // Memory/cache control bits.
    uint32_t i : 1;
    uint32_t m : 1;
    uint32_t g : 1;
    uint32_t reserved2 : 1;
    uint32_t pp : 2;    // Page protection bits.
  };
  /** A BAT table entry - upper half */
  struct BATU
  {
    uint32_t bepi : 15; // Effective page index.
    uint32_t unused : 4;
    uint32_t bl : 11;   // Block size mask.
    uint32_t vs : 1;    // Supervisor valid.
    uint32_t vp : 1;    // User valid.
  };
  /** A BAT table entry - lower half */
  struct BATL
  {
    uint32_t brpn : 15; // Real page index.
    uint32_t unused1 : 10;
    uint32_t wimg : 4;  // Memory/cache control bits.
    uint32_t unused2 : 1;
    uint32_t pp : 2;    // Page access protections.
  };

  /** A page table entry group (PTEG) */
  struct PTEG
  {
    PTE entries[8];
  };

  /** Sets an IBAT entry */
  void setIBAT(size_t n, uintptr_t virt, physical_uintptr_t phys, size_t size, uint32_t mode);
  /** Set a DBAT entry */
  void setDBAT(size_t n, uintptr_t virt, physical_uintptr_t phys, size_t size, uint32_t mode);

  /** Pointer to the HTAB. */
  PTEG *m_pHtab;

  /** The size of the HTAB. */
  uint32_t m_Size;

  /** The mask to use during hashing. */
  uint32_t m_Mask;

  /** The singleton instance. */
  static HashedPageTable m_Instance;
};

#endif
