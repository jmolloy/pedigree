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

#ifndef KERNEL_LINKER_ELF_H
#define KERNEL_LINKER_ELF_H

#include <compiler.h>
#include <processor/types.h>
#include <utilities/List.h>

/** @addtogroup kernellinker
 * @{ */

#ifdef VERBOSE_LINKER
#define DEBUG NOTICE
#else
#define DEBUG(...)
#endif


// Section header types - common to Elf32 and Elf64.
#define SHT_PROGBITS      0x1     // The data is contained in the program file.
#define SHT_SYMTAB        0x2     // Symbol table
#define SHT_STRTAB        0x3     // String table
#define SHT_RELA          0x4
#define SHT_HASH          0x5     // Symbol hash table
#define SHT_DYNAMIC       0x6     // Dynamic linking information
#define SHT_NOTE          0x7
#define SHT_NOBITS        0x8     // The data is not contained in the program file.
#define SHT_REL           0x9
#define SHT_DYNSYM        0xb
#define SHT_INIT_ARRAY    0xe
#define SHT_FINI_ARRAY    0xf
#define SHT_PREINIT_ARRAY 0x10

// Section header flags - common to Elf32 and Elf64.
#define SHF_WRITE         0x1
#define SHF_ALLOC         0x2
#define SHF_EXECINSTR     0x4
#define SHF_MASKPROC      0xf0000000

#ifdef BITS_32

#define ELF32_R_SYM(val)  ((val) >> 8)
#define ELF32_R_TYPE(val) ((val) & 0xff)

#define ELF32_ST_BIND(i)	((i)>>4)
#define ELF32_ST_TYPE(i)	((i)&0xf)
#define ELF32_ST_INFO(b, t)	(((b)<<4)+((t)&0xf))

#define	ELF32_PT_NULL    0 /* Program header table entry unused */
#define ELF32_PT_LOAD    1 /* Loadable program segment */
#define ELF32_PT_DYNAMIC 2 /* Dynamic linking information */
#define ELF32_PT_INTERP  3 /* Program interpreter */
#define ELF32_PT_NOTE    4 /* Auxiliary information */
#define ELF32_PT_SHLIB   5 /* Reserved */
#define ELF32_PT_PHDR    6 /* Entry for header table itself */
#define ELF32_PT_TLS     7 /* Thread-local storage segment */
#define	ELF32_PT_NUM     8 /* Number of defined types */

#define ELF32_DT_NULL    0    /* Marks end of dynamic section */
#define ELF32_DT_NEEDED  1    /* Name of needed library */
#define ELF32_DT_PLTRELSZ  2    /* Size in bytes of PLT relocs */
#define ELF32_DT_PLTGOT  3    /* Processor defined value */
#define ELF32_DT_HASH    4    /* Address of symbol hash table */
#define ELF32_DT_STRTAB  5    /* Address of string table */
#define ELF32_DT_SYMTAB  6    /* Address of symbol table */
#define ELF32_DT_RELA    7    /* Address of Rela relocs */
#define ELF32_DT_RELASZ  8    /* Total size of Rela relocs */
#define ELF32_DT_RELAENT  9    /* Size of one Rela reloc */
#define ELF32_DT_STRSZ  10    /* Size of string table */
#define ELF32_DT_SYMENT  11    /* Size of one symbol table entry */
#define ELF32_DT_INIT    12    /* Address of init function */
#define ELF32_DT_FINI    13    /* Address of termination function */
#define ELF32_DT_SONAME  14    /* Name of shared object */
#define ELF32_DT_RPATH  15    /* Library search path (deprecated) */
#define ELF32_DT_SYMBOLIC  16    /* Start symbol search here */
#define ELF32_DT_REL    17    /* Address of Rel relocs */
#define ELF32_DT_RELSZ  18    /* Total size of Rel relocs */
#define ELF32_DT_RELENT  19    /* Size of one Rel reloc */
#define ELF32_DT_PLTREL  20    /* Type of reloc in PLT */
#define ELF32_DT_DEBUG  21    /* For debugging; unspecified */
#define ELF32_DT_TEXTREL  22    /* Reloc might modify .text */
#define ELF32_DT_JMPREL  23    /* Address of PLT relocs */
#define ELF32_DT_BIND_NOW  24    /* Process relocations of object */
#define ELF32_DT_INIT_ARRAY  25    /* Array with addresses of init fct */
#define ELF32_DT_FINI_ARRAY  26    /* Array with addresses of fini fct */
#define ELF32_DT_INIT_ARRAYSZ  27    /* Size in bytes of ELF32_DT_INIT_ARRAY */
#define ELF32_DT_FINI_ARRAYSZ  28    /* Size in bytes of ELF32_DT_FINI_ARRAY */
#define ELF32_DT_RUNPATH  29    /* Library search path */
#define ELF32_DT_FLAGS  30    /* Flags for the object being loaded */
#define ELF32_DT_ENCODING  32    /* Start of encoded range */
#define ELF32_DT_PREINIT_ARRAY 32    /* Array with addresses of preinit fct*/
#define ELF32_DT_PREINIT_ARRAYSZ 33    /* size in bytes of ELF32_DT_PREINIT_ARRAY */

#endif
#ifdef BITS_64

#define ELF64_R_SYM(val)  ((val) >> 8)
#define ELF64_R_TYPE(val) ((val) & 0xff)

#define ELF64_ST_BIND(i)        ((i)>>4)
#define ELF64_ST_TYPE(i)        ((i)&0xf)
#define ELF64_ST_INFO(b, t)     (((b)<<4)+((t)&0xf))



#endif

/**
 * Provides an implementation of a 32-bit Executable and Linker format file parser.
 * The ELF data can be loaded either by supplying an entire ELF file in a buffer, or
 * by supplying details of each section seperately.
 * \note This class does not copy any section data. The supplied buffer pointers must
 *       remain valid throughout the lifespan of this class.
 */
class Elf
{
public:
  /**
   * A callback type for symbol lookup. This is used by the dynamic linking functions.
   */
  typedef uintptr_t (*SymbolLookupFn)(const char *, bool useElf);

  /**
   * Default constructor - loads no data.
   */
  Elf();

  /**
   * Destructor.
   */
  virtual ~Elf();

  /**
   * Constructs an Elf object, and assumes the given pointer to be
   * to a contiguous region of memory containing an ELF object. 
   * 
   * All important information in the buffer is cached - the buffer can safely be destroyed after
   * calling this function.
   */
  bool create(uint8_t *pBuffer, size_t length);

  /**
   * Maps memory at a specified address, loads code there and applies relocations only for the .modinfo section.
   * Intended use is for loading kernel modules only, there is no provision for dynamic relocations.
   */
  bool loadModule(uint8_t *pBuffer, size_t length, uintptr_t &loadBase);

  /**
   * Finalises a module - applies all relocations except those in the .modinfo section. At this point it is
   * assumed that all of this module's dependencies have been loaded.
   *
   * The load base must be given again for reentrancy reasons.
   */
  bool finaliseModule(uint8_t *pBuffer, uint32_t length);

  /**
   * Performs the prerequisite allocation for any normal ELF file - library or executable.
   * For a library, this allocates loadBase, and allocates memory for the entire object - this is not
   * filled however.
   */
  bool allocate(uint8_t *pBuffer, size_t length, uintptr_t &loadBase, class Process *pProcess=0);
  
  /**
   * Loads (part) of a 'normal' file. This could be an executable or a library. By default the entire file
   * is loaded (memory copied and relocated) but this can be changed using the nStart and nEnd
   * parameters. This allows for lazy loading.
   * \note PLT relocations are not performed here - they are defined in a different section to the standard Rel
   * and RELA entries, so must be done specifically (via applySpecificRelocation).
   */
  bool load(uint8_t *pBuffer, size_t length, uintptr_t loadBase, SymbolLookupFn fn=0, uintptr_t nStart=0, uintptr_t nEnd=~0);


  /**
   * Returns a list of required libraries before this object will load.
   */
  List<char*> &neededLibraries();

  /**
   * Returns the virtual address of the last byte to be written. Used to calculate the
   * sbrk memory breakpoint.
   */
  uintptr_t getLastAddress();

  /**
   * Returns the name of the symbol which contains 'addr', and also the starting address
   * of that symbol in 'startAddr' if startAddr != 0.
   * \param[in] addr The address to look up.
   * \param[out] startAddr The starting address of the found symbol (optional).
   * \return The symbol name, as a C string.
   */
  const char *lookupSymbol(uintptr_t addr, uintptr_t *startAddr=0);

  /**
   * Returns the start address of the symbol with name 'pName'.
   */
  uint32_t lookupSymbol(const char *pName);

  /**
   * Same as lookupSymbol, but acts on the dynamic symbol table instead of the normal one.
   */
  uintptr_t lookupDynamicSymbolAddress(const char *str, uintptr_t loadBase);
  
  /**
   * Applies the n'th relocation in the relocation table. Used by PLT entries.
   */
  uintptr_t applySpecificRelocation(uint32_t off, SymbolLookupFn fn, uintptr_t loadBase);

  /**
   * Gets the address of the global offset table.
   * \return Address of the GOT, or 0 if none was found.
   */
  uintptr_t getGlobalOffsetTable();


  size_t getPltSize ();

  /**
   * Returns the entry point of the file.
   */
  uintptr_t getEntryPoint();

  uintptr_t debugFrameTable();
  uintptr_t debugFrameTableLength();

protected:

#ifdef BITS_32
  struct ElfHeader_t
  {
    uint8_t  ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
  } PACKED;

  struct ElfProgramHeader_t
  {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
  } PACKED;

  struct ElfSectionHeader_t
  {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
  } PACKED;

  struct ElfSymbol_t
  {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    uint8_t  info;
    uint8_t  other;
    uint16_t shndx;
  } PACKED;

  struct ElfDyn_t
  {
    int32_t tag;
    union
    {
      int32_t val;
      uint32_t ptr;
    } un;
  } PACKED;

  struct ElfRel_t
  {
    uint32_t offset;
    uint32_t info;
  } PACKED;

  struct ElfRela_t
  {
    uint32_t offset;
    uint32_t info;
    uint32_t addend;
  } PACKED;
#endif // BITS_32

private:

  bool relocate(uint8_t *pBuffer, uint32_t length);
  bool relocateModinfo(uint8_t *pBuffer, uint32_t length);
  
  /**
   * Applies one relocation. This overload performs a relocation without addend (REL).
   * \param rel The relocation entry to apply.
   * \param pSh A pointer to the section that the relocation entry refers to.
   * \param fn  A function pointer to a function that, given a string, looks up an address. If NULL, the KernelElf is consulted.
   * \note Defined in core/processor/.../Elf32.cc
   */
  bool applyRelocation(ElfRel_t rel, ElfSectionHeader_t *pSh, SymbolLookupFn fn=0, uintptr_t loadBase=0);

  /**
   * Applies one relocation. This overload performs a relocation with addend (RELA).
   * \param rel The relocation entry to apply.
   * \param pSh A pointer to the section that the relocation entry refers to.
   * \param fn  A function pointer to a function that, given a string, looks up an address. If NULL, the KernelElf is consulted.
   * \note Defined in core/processor/.../Elf32.cc
   */
  bool applyRelocation(ElfRela_t rela, ElfSectionHeader_t *pSh, SymbolLookupFn fn=0, uintptr_t loadBase=0);

protected:
  ElfSymbol_t          *m_pSymbolTable;
  size_t                m_nSymbolTableSize;
  char                 *m_pStringTable;
  char                 *m_pShstrtab;
  uintptr_t            *m_pGotTable; // Global offset table.
  ElfRel_t             *m_pRelTable; // Dynamic REL relocations.
  ElfRela_t            *m_pRelaTable; // Dynamic RELA relocations.
  uint32_t              m_nRelTableSize;
  uint32_t              m_nRelaTableSize;
  ElfRel_t             *m_pPltRelTable;
  ElfRela_t            *m_pPltRelaTable;
  bool                  m_bUsesRela; // If PltRelaTable is valid, else PltRelTable is.
  uint8_t              *m_pDebugTable;
  uintptr_t             m_nDebugTableSize;
  ElfSymbol_t          *m_pDynamicSymbolTable;
  size_t                m_nDynamicSymbolTableSize;
  char                 *m_pDynamicStringTable;
  ElfSectionHeader_t   *m_pSectionHeaders;
  uint32_t              m_nSectionHeaders;
  ElfProgramHeader_t   *m_pProgramHeaders;
  uint32_t              m_nProgramHeaders;
  size_t                m_nPltSize;
  uintptr_t             m_nEntry;
  List<char*>           m_NeededLibraries;

private:
  /** The copy-constructor
   *\note currently not implemented */
  Elf(const Elf &);
  /** The assignment operator
   *\note currently not implemented */
  Elf &operator = (const Elf &);
};

/** @} */

#endif
