/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#ifndef KERNEL_LINKER_KERNELELF_H
#define KERNEL_LINKER_KERNELELF_H

#include <Elf32.h>
#include <Elf64.h>
#include <compiler.h>
#include <processor/MemoryRegion.h>
#include <BootstrapInfo.h>

class KernelElf
  #if defined(BITS_64)
    : public Elf64
  #elif defined(BITS_32)
    : public Elf32
  #endif
{
  public:
    /** Get the class instance
     *\return reference to the class instance */
    inline static KernelElf &instance()
      {return m_Instance;}

    /** Extracts the symbol and string tables from the given BootstrapInfo class. */
    bool initialise(const BootstrapStruct_t &pBootstrap) INITIALISATION_ONLY;

  private:
    /** Default constructor does nothing */
    KernelElf() INITIALISATION_ONLY;
    /** Destructor does nothing */
    ~KernelElf();
    /** Copy-constructor
     *\note NOT implemented (singleton class) */
    KernelElf(const KernelElf &);
    /** Assignment operator
     *\note NOT implemented (singleton class) */
    KernelElf &operator = (const KernelElf &);

    #if defined(BITS_64)
      typedef Elf64SectionHeader_t ElfSectionHeader_t;
    #elif defined(BITS_32)
      typedef Elf32SectionHeader_t ElfSectionHeader_t;
    #endif

    MemoryRegion m_AdditionalSections;

    /** Instance of the KernelElf class */
    static KernelElf m_Instance;
};

#endif
