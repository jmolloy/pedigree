#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define SHT_REL           0x9
#define R_SYM(val)  ((val) >> 8)
#define R_TYPE(val) ((val) & 0xff)
#define ST_TYPE(i)  ((i)&0xf)

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

typedef struct ElfRel_t
{
    Elf_Addr offset;
    Elf_Xword info;
} __attribute__((packed)) ElfRel_t;

typedef struct Elf_t
{
    uint8_t *buffer;
    ElfHeader_t *header;
    ElfSectionHeader_t *section_headers;
    ElfSymbol_t *symtab;
    size_t symtabsz;
    char *strtab;
    size_t strtabsz;
} Elf_t;

#define SYM_PTR(elf, sym) ((uint32_t*)&(elf->buffer[sym->value + elf->section_headers[sym->shndx].offset]))

Elf_t *elf_create(uint8_t *buffer, size_t len);
ElfSymbol_t *elf_get_symbol(Elf_t *elf, const char *name);
uint8_t *elf_relptr(Elf_t *elf, ElfSymbol_t *sym, uint32_t ptr);

#endif
