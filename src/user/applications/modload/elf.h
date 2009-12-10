#ifndef ELF_H
#define ELF_H

#include <stdint.h>

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
#define PT_NULL    0 /* Program header table entry unused */
#define PT_LOAD    1 /* Loadable program segment */
#define PT_DYNAMIC 2 /* Dynamic linking information */
#define PT_INTERP  3 /* Program interpreter */
#define PT_NOTE    4 /* Auxiliary information */
#define PT_SHLIB   5 /* Reserved */
#define PT_PHDR    6 /* Entry for header table itself */
#define PT_TLS     7 /* Thread-local storage segment */
#define PT_NUM     8 /* Number of defined types */

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

#define R_SYM(val)  ((val) >> 8)
#define R_TYPE(val) ((val) & 0xff)

#define ST_BIND(i)  ((i)>>4)
#define ST_TYPE(i)  ((i)&0xf)
#define ST_INFO(b, t)   (((b)<<4)+((t)&0xf))

typedef uint32_t Elf_Addr;
typedef uint32_t Elf_Off;
typedef uint16_t Elf_Half;
typedef uint32_t Elf_Word;
typedef  int32_t Elf_Sword;

// We define the Xword and Sxword types for ELF32 even though they don't exist
// in the spec for forwards compatibility with ELF64.
typedef uint32_t Elf_Xword;
typedef  int32_t Elf_Sxword;

typedef struct ElfHeader_t
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
} __attribute__((packed)) ElfHeader_t;

typedef struct ElfProgramHeader_t
{
    Elf_Word type;
    Elf_Off offset;
    Elf_Addr vaddr;
    Elf_Addr paddr;
    Elf_Xword filesz;
    Elf_Xword memsz;
    Elf_Word flags;
    Elf_Xword align;
} __attribute__((packed)) ElfProgramHeader_t;

typedef struct ElfSectionHeader_t
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
} __attribute__((packed)) ElfSectionHeader_t;

typedef struct ElfSymbol_t
{
    Elf_Word name;
    Elf_Addr value;
    Elf_Xword size;
    uint8_t  info;
    uint8_t  other;
    Elf_Half shndx;
} __attribute__((packed)) ElfSymbol_t;

typedef struct ElfDyn_t
{
    Elf_Sxword tag;
    union
    {
        Elf_Xword val;
        Elf_Addr ptr;
    } un;
} __attribute__((packed)) ElfDyn_t;

typedef struct ElfRel_t
{
    Elf_Addr offset;
    Elf_Xword info;
} __attribute__((packed)) ElfRel_t;

typedef struct ElfRela_t
{
    Elf_Addr offset;
    Elf_Xword info;
    Elf_Sxword addend;
} __attribute__((packed)) ElfRela_t;

typedef struct Elf_t
{
    uint8_t *buffer;
    ElfHeader_t *header;
    ElfSectionHeader_t *section_headers;
    ElfSymbol_t *symtab;
    size_t symtabsz;
    char *strtab;
    size_t strtabsz
} Elf_t;

#define SYM_PTR(elf, sym) ((uint32_t*)&(elf->buffer[sym->value + elf->section_headers[sym->shndx].offset]))

Elf_t *elf_create(uint8_t *buffer, size_t len);
ElfSymbol_t *elf_get_symbol(Elf_t *elf, const char *name);
uint8_t *elf_relptr(Elf_t *elf, ElfSymbol_t *sym, uint32_t ptr);

#endif
