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

#include "FatFile.h"
#include "FatFilesystem.h"

FatFile::FatFile(String name, Time accessedTime, Time modifiedTime, Time creationTime,
       uintptr_t inode, class Filesystem *pFs, size_t size, uint32_t dirClus, uint32_t dirOffset, File *pParent) :
  File(name,accessedTime,modifiedTime,creationTime,inode,pFs,size,pParent),
  m_DirClus(dirClus), m_DirOffset(dirOffset)
{
}

FatFile::~FatFile()
{
}

uintptr_t FatFile::readBlock(uint64_t location)
{
    /// \note Not freed. Watch out.
    uintptr_t buffer = reinterpret_cast<uintptr_t>(new uint8_t[1024]);
    FatFilesystem *pFs = reinterpret_cast<FatFilesystem*>(m_pFilesystem);
    pFs->read(this, location, 1024, buffer);
    return buffer;
}
