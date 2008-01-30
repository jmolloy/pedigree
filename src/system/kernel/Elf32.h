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

#ifndef ELF32_H
#define ELF32_H

#include <machine/types.h>

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

#define SHF_WRITE         0x1
#define SHF_ALLOC         0x2
#define SHF_EXECINSTR     0x4
#define SHF_MASKPROC      0xf0000000

#define ELF32_R_SYM(val)  ((val) >> 8)
#define ELF32_R_TYPE(val) ((val) & 0xff)

/**
 * Provides an implementation of a 32-bit Executable and Linker format file parser.
 * The ELF data can be loaded either by supplying an entire ELF file in a buffer, or
 * by supplying details of each section seperately.
 * \note This class does not copy any section data. The supplied buffer pointers must
 *       remain valid throughout the lifespan of this class.
 */
class Elf32
{
public:
  /**
   * Default constructor - loads no data.
   */
  Elf32();
  /**
   * Constructs an Elf32 object, and assumes the given pointer to be to a contiguous region
   * of memory containing an ELF object.
   */
  Elf32(uint8_t *pBuffer);
  /**
   * Destructor. Doesn't do much.
   */
  ~Elf32();

  /**
   * Loads a section header.
   * \param pPtr pointer to the section header, in memory.
   */
  bool loadSectionHeader(void *pPtr);

  /**
   * Writes all writeable sections to their virtual addresses.
   * \return True on success.
   */
  bool writeSections();

  /**
   * Returns the virtual address of the last byte to be written. Used to calculate the 
   * sbrk memory breakpoint.
   */
  unsigned int getLastAddress();

  /**
   * Returns the name of the symbol which contains 'addr', and also the starting address
   * of that symbol in 'startAddr' if startAddr != 0.
   * \param[in] addr The address to look up.
   * \param[out] startAddr The starting address of the found symbol (optional).
   * \return The symbol name, as a C string.
   */
  char *lookupSymbol(uint32_t addr, uint32_t startAddr=0);

  /**
   * Returns the address of the symbol with offset 'off' in the dynamic relocation table.
   */
  uint32_t lookupDynamicSymbolAddress(uint32_t off);

  /**
   * Returns a NULL terminated string specifying the name of 
   * the symbol at offset 'off' in the relocation symbol table.
   */
  char *lookupDynamicSymbolName(uint32_t off);

  /**
   * Gets the address of the global offset table.
   * \return Address of the GOT, or 0 if none was found.
   */
  uint32_t getGlobalOffsetTable();

  /**
   * Returns the entry point of the file.
   */
  uint32_t getEntryPoint();

private:
  typedef struct
  {
    u8int  ident[16];
    u16int type;
    u16int machine;
    u32int version;
    u32int entry;
    u32int phoff;
    u32int shoff;
    u32int flags;
    u16int ehsize;
    u16int phentsize;
    u16int phnum;
    u16int shentsize;
    u16int shnum;
    u16int shstrndx;
  } Elf32Header_t;
  
  typedef struct
  {
    u32int type;
    u32int offset;
    u32int vaddr;
    u32int paddr;
    u32int filesz;
    u32int memsz;
    u32int flags;
    u32int align;
  } Elf32ProcessHeader_t;
  
  typedef struct
  {
    u32int name;
    u32int type;
    u32int flags;
    u32int addr;
    u32int offset;
    u32int size;
    u32int link;
    u32int info;
    u32int addralign;
    u32int entsize;
  } Elf32SectionHeader_t;
  
  typedef struct
  {
    u32int name;
    u32int value;
    u32int size;
    u8int  info;
    u8int  other;
    u16int shndx;
  } Elf32Symbol_t;
  
  typedef struct
  {
    s32int tag;
    union
    {
      s32int val;
      u32int ptr;
    } un;
  } Elf32Dyn_t;
  
  typedef struct
  {
    u32int offset;
    u32int info;
  } Elf32Rel_t;

};


#endif
