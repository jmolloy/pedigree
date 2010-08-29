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
#ifndef RAWFS_FILE_H
#define RAWFS_FILE_H

#include <vfs/File.h>
#include <utilities/String.h>
#include <machine/Disk.h>

class RawFsFile : public File
{
private:
    /** Copy constructors are hidden - unused! */
    RawFsFile(const RawFsFile &file);
    RawFsFile& operator =(const RawFsFile&);
public:
    /** Constructor, should only be called by RawFs. */
    RawFsFile(String name, class RawFs *pFs, File *pParent, Disk *pDisk);
    ~RawFsFile() {}
    
    virtual uintptr_t readBlock(uint64_t location);

    virtual void fileAttributeChanged() {}
private:
    Disk *m_pDisk;
};

#endif
