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

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <processor/types.h>
#include <processor/MemoryRegion.h>

/**
 * This class provides functions for extracting an archive file as made by UNIX Tar.
 */
class Archive
{
public:
  
  /** Constructor; takes the physical address of a Tar archive. */
  Archive(uint8_t *pPhys);
  /** Destructor; returns the physical memory used during construction to the
   *  physical memory pool. */
  ~Archive();

  /** Returns the number of files in the archive. */
  size_t getNumFiles();

  /** Returns the size of the n'th file in the archive, in bytes.
   *  \param n The file to retrieve. */
  size_t getFileSize(size_t n);

  /** Returns the name of the n'th file in the archive, in bytes.
   *  \param n The file to retrieve. */
  char *getFileName(size_t n);

  /** Returns a pointer to the first byte of the n'th file.
   *  \param n The file to retrieve. */
  uintptr_t *getFile(size_t n);

private:
  struct File
  {
    char name[100];         // Filename.
    char mode[8];           // Permissions.
    char uid[8];            // User ID.
    char gid[8];            // Group ID.
    char size[12];          // Size, in bytes (octal)
    char mtime[12];         // Modification time.
    char chksum[8];         // Header checksum.
    char link;              // Link type: 0 = regular file, 1 = hard link, 2 = symlink.
    char linkname[100];     // Linked-to file name.
  };

  uint8_t *m_pPhysicalAddress;
  uint8_t *m_pVirtualAddress;
  MemoryRegion m_Region;
};

#endif
