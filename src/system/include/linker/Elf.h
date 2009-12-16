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
#include <linker/SymbolTable.h>

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

// Process header flags - common to Elf32 and Elf64.
#define	PT_NULL    0 /* Program header table entry unused */
#define PT_LOAD    1 /* Loadable program segment */
#define PT_DYNAMIC 2 /* Dynamic linking information */
#define PT_INTERP  3 /* Program interpreter */
#define PT_NOTE    4 /* Auxiliary information */
#define PT_SHLIB   5 /* Reserved */
#define PT_PHDR    6 /* Entry for header table itself */
#define PT_TLS     7 /* Thread-local storage segment */
#define	PT_NUM     8 /* Number of defined types */

// Dynamic table flags - common to Elf32 and Elf64.
#define DT_NULL         0    /* Marks end of dynamic section */
#define DT_NEEDED       1    /* Name of needed library */
#define DT_PLTRELSZ     2    /* Size in bytes of PLT relocs */
#define DT_PLTGOT       3    /* Processor defined value */
#define DT_HASH         4    /* Address of symbol hash table */
#define DT_STRTAB       5    /* Address of string table */
#define DT_SYMTAB       6    /* Address of symbol table */
#define DT_RELA         7    /* Address of Rela relocs */
#define DT_RELASZ       8    /* Total size of Rela relocs */
#define DT_RELAENT      9    /* Size of one Rela reloc */
#define DT_STRSZ       10    /* Size of string table */
#define DT_SYMENT      11    /* Size of one symbol table entry */
#define DT_INIT        12    /* Address of init function */
#define DT_FINI        13    /* Address of termination function */
#define DT_SONAME      14    /* Name of shared object */
#define DT_RPATH       15    /* Library search path (deprecated) */
#define DT_SYMBOLIC    16    /* Start symbol search here */
#define DT_REL         17    /* Address of Rel relocs */
#define DT_RELSZ       18    /* Total size of Rel relocs */
#define DT_RELENT      19    /* Size of one Rel reloc */
#define DT_PLTREL      20    /* Type of reloc in PLT */
#define DT_DEBUG       21    /* For debugging; unspecified */
#define DT_TEXTREL     22    /* Reloc might modify .text */
#define DT_JMPREL      23    /* Address of PLT relocs */
#define DT_BIND_NOW    24    /* Process relocations of object */
#define DT_INIT_ARRAY  25    /* Array with addresses of init fct */
#define DT_FINI_ARRAY  26    /* Array with addresses of fini fct */
#define DT_INIT_ARRAYSZ 27   /* Size in bytes of DT_INIT_ARRAY */
#define DT_FINI_ARRAYSZ 28   /* Size in bytes of DT_FINI_ARRAY */
#define DT_RUNPATH     29    /* Library search path */
#define DT_FLAGS       30    /* Flags for the object being loaded */
#define DT_ENCODING    32    /* Start of encoded range */
#define DT_PREINIT_ARRAY 32  /* Array with addresses of preinit fct*/
#define DT_PREINIT_ARRAYSZ 33 /* size in bytes of DT_PREINIT_ARRAY */


#ifdef BITS_32

#define R_SYM(val)  ((val) >> 8)
#define R_TYPE(val) ((val) & 0xff)

#define ST_BIND(i)	((i)>>4)
#define ST_TYPE(i)	((i)&0xf)
#define ST_INFO(b, t)	(((b)<<4)+((t)&0xf))

typedef uint32_t Elf_Addr;
typedef uint32_t Elf_Off;
typedef uint16_t Elf_Half;
typedef uint32_t Elf_Word;
typedef  int32_t Elf_Sword;

// We define the Xword and Sxword types for ELF32 even though they don't exist
// in the spec for forwards compatibility with ELF64.
typedef uint32_t Elf_Xword;
typedef  int32_t Elf_Sxword;

#endif
#ifdef BITS_64

#define R_SYM(val)  ((val) >> 32)
#define R_TYPE(val) ((val) & 0xffffffffUL)

#define ST_BIND(i)        ((i)>>4)
#define ST_TYPE(i)        ((i)&0xf)
#define ST_INFO(b, t)     (((b)<<4)+((t)&0xf))

typedef uint64_t Elf_Addr;
typedef uint64_t Elf_Off;
typedef uint16_t Elf_Half;
typedef uint32_t Elf_Word;
typedef  int32_t Elf_Sword;
typedef uint64_t Elf_Xword;
typedef  int64_t Elf_Sxword;

#endif

/**
 * Provides an implementation of a 32-bit Executable and Linker format file parser.
 * The ELF data can be loaded either by supplying an entire ELF file in a buffer, or
 * by supplying details of each section seperately.
 */
class Elf
{
    public:
        /** Default constructor - loads no data. */
        Elf();

        /** Destructor.*/
        virtual ~Elf();

        /** The copy-constructor */
        Elf(const Elf &);

        /** Constructs an Elf object, and assumes the given pointer to be
        * to a contiguous region of memory containing an ELF object. */
        bool create(uint8_t *pBuffer, size_t length);

        /** Merely loads "needed libraries" and then returns success or
        *  failure. */
        bool createNeededOnly(uint8_t *pBuffer, size_t length);

        /** Maps memory at a specified address, loads code there and applies relocations only for the .modinfo section.
        * Intended use is for loading kernel modules only, there is no provision for dynamic relocations. */
        bool loadModule(uint8_t *pBuffer, size_t length, uintptr_t &loadBase, size_t &loadSize, SymbolTable *pSymbolTableCopy=0);

        /** Finalises a module - applies all relocations except those in the .modinfo section. At this point it is
        * assumed that all of this module's dependencies have been loaded.
        *
        * The load base must be given again for reentrancy reasons. */
        bool finaliseModule(uint8_t *pBuffer, uintptr_t length);

        /** Performs the prerequisite allocation for any normal ELF file - library or executable.
        * For a library, this allocates loadBase, and allocates memory for the entire object - this is not
        * filled however.
        * \note If bAllocate is false, the memory will NOT be allocated. */
        bool allocate(uint8_t *pBuffer, size_t length, uintptr_t &loadBase, SymbolTable *pSymtab=0, bool bAllocate=true, size_t *pSize=0);

        /** Loads (part) of a 'normal' file. This could be an executable or a library. By default the entire file
        * is loaded (memory copied and relocated) but this can be changed using the nStart and nEnd
        * parameters. This allows for lazy loading.
        * \note PLT relocations are not performed here - they are defined in a different section to the standard REL
        * and RELA entries, so must be done specifically (via applySpecificRelocation). */
        bool load(uint8_t *pBuffer, size_t length, uintptr_t loadBase, SymbolTable *pSymtab=0, uintptr_t nStart=0, uintptr_t nEnd=~0);


        /** Returns a list of required libraries before this object will load. */
        List<char*> &neededLibraries();

        /** Returns the virtual address of the last byte to be written. Used to calculate the
        * sbrk memory breakpoint. */
        uintptr_t getLastAddress();

        uintptr_t getInitFunc() {return m_InitFunc;}
        uintptr_t getFiniFunc() {return m_FiniFunc;}

        /** Returns the name of the symbol which contains 'addr', and also the starting address
        * of that symbol in 'startAddr' if startAddr != 0.
        * \param[in] addr The address to look up.
        * \param[out] startAddr The starting address of the found symbol (optional).
        * \return The symbol name, as a C string. */
        const char *lookupSymbol(uintptr_t addr, uintptr_t *startAddr=0);

        /** Returns the start address of the symbol with name 'pName'. */
        uintptr_t lookupSymbol(const char *pName);

        /** Same as lookupSymbol, but acts on the dynamic symbol table instead of the normal one. */
        uintptr_t lookupDynamicSymbolAddress(const char *str, uintptr_t loadBase);

        /** Applies the n'th relocation in the relocation table. Used by PLT entries. */
        uintptr_t applySpecificRelocation(uintptr_t off, SymbolTable *pSymtab, uintptr_t loadBase, SymbolTable::Policy policy = SymbolTable::LocalFirst);

        /** Gets the address of the global offset table.
        * \return Address of the GOT, or 0 if none was found. */
        uintptr_t getGlobalOffsetTable();

        /** Returns the size of the Procedure Linkage table. */
        size_t getPltSize ();

        /**  Adds all the symbols in this Elf into the given symbol table, adjusted by loadBase.
        *
        * \param pSymtab  Symbol table to populate.
        * \param loadBase Offset to adjust each value by. */
        void populateSymbolTable(SymbolTable *pSymtab, uintptr_t loadBase);

        SymbolTable *getSymbolTable()
        {
            return &m_SymbolTable;
        }

        /** Returns the entry point of the file. */
        uintptr_t getEntryPoint();

        uintptr_t debugFrameTable();
        uintptr_t debugFrameTableLength();

    protected:

        struct ElfHeader_t
        {
            uint8_t  ident[16];
            Elf_Half type;
            Elf_Half machine;
            Elf_Word version;
            Elf_Addr entry;
            Elf_Off  phoff;
            Elf_Off  shoff;
            Elf_Word flags;
            Elf_Half ehsize;
            Elf_Half phentsize;
            Elf_Half phnum;
            Elf_Half shentsize;
            Elf_Half shnum;
            Elf_Half shstrndx;
        } PACKED;

        struct ElfProgramHeader_t
        {
            Elf_Word type;
            #ifdef BITS_64
            Elf_Word flags;
            #endif
            Elf_Off offset;
            Elf_Addr vaddr;
            Elf_Addr paddr;
            Elf_Xword filesz;
            Elf_Xword memsz;
            #ifndef BITS_64
            Elf_Word flags;
            #endif
            Elf_Xword align;
        } PACKED;

        struct ElfSectionHeader_t
        {
            Elf_Word name;
            Elf_Word type;
            Elf_Xword flags;
            Elf_Addr addr;
            Elf_Off offset;
            Elf_Xword size;
            Elf_Word link;
            Elf_Word info;
            Elf_Xword addralign;
            Elf_Xword entsize;
        } PACKED;

        struct ElfSymbol_t
        {
            Elf_Word name;
            #ifdef BITS_64
            uint8_t  info;
            uint8_t  other;
            Elf_Half shndx;
            #endif
            Elf_Addr value;
            Elf_Xword size;
            #ifndef BITS_64
            uint8_t  info;
            uint8_t  other;
            Elf_Half shndx;
            #endif
        } PACKED;

        struct ElfDyn_t
        {
            Elf_Sxword tag;
            union
            {
                Elf_Xword val;
                Elf_Addr ptr;
            } un;
        } PACKED;

        struct ElfRel_t
        {
            Elf_Addr offset;
            Elf_Xword info;
        } PACKED;

        struct ElfRela_t
        {
            Elf_Addr offset;
            Elf_Xword info;
            Elf_Sxword addend;
        } PACKED;

    private:

        bool relocate(uint8_t *pBuffer, uintptr_t length);
        bool relocateModinfo(uint8_t *pBuffer, uintptr_t length);

        /**
        * Applies one relocation. This overload performs a relocation without addend (REL).
        * \param rel The relocation entry to apply.
        * \param pSh A pointer to the section that the relocation entry refers to.
        * \param pSymtab The symbol table to use for lookups.
        * \param loadBase For a relocatable object, the address at which it is loaded.
        * \param policy Lookup policy.
        * \note Defined in core/processor/.../Elf.cc
        */
        bool applyRelocation(ElfRel_t rel, ElfSectionHeader_t *pSh, SymbolTable *pSymtab=0, uintptr_t loadBase=0, SymbolTable::Policy policy = SymbolTable::LocalFirst);

        /**
        * Applies one relocation. This overload performs a relocation with addend (RELA).
        * \param rel The relocation entry to apply.
        * \param pSh A pointer to the section that the relocation entry refers to.
        * \param pSymtab The symbol table to use for lookups.
        * \param loadBase For a relocatable object, the address at which it is loaded.
        * \param policy Lookup policy.
        * \note Defined in core/processor/.../Elf.cc
        */
        bool applyRelocation(ElfRela_t rela, ElfSectionHeader_t *pSh, SymbolTable *pSymtab=0, uintptr_t loadBase=0, SymbolTable::Policy policy = SymbolTable::LocalFirst);

    protected:
        ElfSymbol_t          *m_pSymbolTable;
        size_t                m_nSymbolTableSize;
        char                 *m_pStringTable;
        size_t                m_nStringTableSize;
        char                 *m_pShstrtab;
        size_t                m_nShstrtabSize;
        uintptr_t            *m_pGotTable; // Global offset table.
        ElfRel_t             *m_pRelTable; // Dynamic REL relocations.
        ElfRela_t            *m_pRelaTable; // Dynamic RELA relocations.
        size_t                m_nRelTableSize;
        size_t                m_nRelaTableSize;
        ElfRel_t             *m_pPltRelTable;
        ElfRela_t            *m_pPltRelaTable;
        bool                  m_bUsesRela; // If PltRelaTable is valid, else PltRelTable is.
        uint8_t              *m_pDebugTable;
        size_t                m_nDebugTableSize;
        ElfSymbol_t          *m_pDynamicSymbolTable;
        size_t                m_nDynamicSymbolTableSize;
        char                 *m_pDynamicStringTable;
        size_t                m_nDynamicStringTableSize;
        ElfSectionHeader_t   *m_pSectionHeaders;
        size_t                m_nSectionHeaders;
        ElfProgramHeader_t   *m_pProgramHeaders;
        size_t                m_nProgramHeaders;
        size_t                m_nPltSize;
        uintptr_t             m_nEntry;
        List<char*>           m_NeededLibraries;
        SymbolTable           m_SymbolTable;
        uintptr_t             m_InitFunc;
        uintptr_t             m_FiniFunc;

    private:
        /** The assignment operator
        *\note currently not implemented */
        Elf &operator = (const Elf &);
};

/** @} */

#endif
