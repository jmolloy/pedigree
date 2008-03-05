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

#ifndef BOOTSTRAPINFO_H
#define BOOTSTRAPINFO_H

#include <compiler.h>
#include <processor/types.h>

struct BootstrapStruct_t
{
  // If we are passed via grub, this information will be completely different to
  // via the bootstrapper.
#if defined(KERNEL_STANDALONE) || defined(MIPS_COMMON)
  uint32_t flags;
  
  uint32_t mem_lower;
  uint32_t mem_upper;
  
  uint32_t boot_device;
  
  uint32_t cmdline;
  
  uint32_t mods_count;
  uint32_t mods_addr;
  
  /* ELF information */
  uint32_t num;
  uint32_t size;
  uint32_t addr;
  uint32_t shndx;
  
  uint32_t mmap_length;
  uint32_t mmap_addr;
  
  uint32_t drives_length;
  uint32_t drives_addr;
  
  uint32_t config_table;
  
  uint32_t boot_loader_name;
  
  uint32_t apm_table;
  
  uint32_t vbe_control_info;
  uint32_t vbe_mode_info;
  uint32_t vbe_mode;
  uint32_t vbe_interface_seg;
  uint32_t vbe_interface_off;
  uint32_t vbe_interface_len;
#else // KERNEL_STANDALONE

#endif // !KERNEL_STANDALONE
} PACKED;

/**
 * Parser for a boot-time structure detailing the whereabouts of e.g.
 * the kernel symbol table, and a memory map.
 * \note This class makes a copy of the given BootstrapStruct.
 */
class BootstrapInfo
{
public:
  /**
   * Constructor. Should initialise the m_BootstrapInfo member, and not much else.
   */
  BootstrapInfo(BootstrapStruct_t *info);

  /**
   * Destructor. Shouldn't do anything.
   */
  ~BootstrapInfo();

  /**
   * Retrieves the number of kernel ELF section headers.
   */
  int getSectionHeaderCount();
  
  /**
   * Retrieves a pointer to the n'th kernel ELF section header.
   *
   * Assumes n is less than getSectionHeaderCount().
   */
  uint8_t *getSectionHeader(int n);
  
  /**
   * Retrieves an index, suitable for use with getSectionHeader, to access
   * the String Table section of the kernel ELF header.
   */
  int getStringTable();
  
private:
  BootstrapStruct_t m_BootstrapInfo;
  
};

// Again, if we're passed via grub these multiboot #defines will be valid, otherwise they won't.
#ifdef KERNEL_STANDALONE
#define MULTIBOOT_FLAG_MEM     0x001
#define MULTIBOOT_FLAG_DEVICE  0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS    0x008
#define MULTIBOOT_FLAG_AOUT    0x010
#define MULTIBOOT_FLAG_ELF     0x020
#define MULTIBOOT_FLAG_MMAP    0x040
#define MULTIBOOT_FLAG_CONFIG  0x080
#define MULTIBOOT_FLAG_LOADER  0x100
#define MULTIBOOT_FLAG_APM     0x200
#define MULTIBOOT_FLAG_VBE     0x400
#endif

#endif
