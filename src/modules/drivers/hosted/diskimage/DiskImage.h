/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#include <machine/Disk.h>

/** Loads a disk image as a usable disk device. */
class DiskImage : public Disk
{
public:
    DiskImage() : Disk(), m_pBase(0), m_nSize(0)
    {
    }

    virtual ~DiskImage()
    {
    }

    bool initialise();

    virtual void getName(String &str)
    {
        str = "Hosted disk image";
    }

    virtual void dump(String &str)
    {
        str = "Hosted disk image";
    }

    virtual uintptr_t read(uint64_t location);

    virtual size_t getSize() const;

    virtual size_t getBlockSize() const
    {
        return 4096;
    }
private:
    void *m_pBase;
    size_t m_nSize;
};

#endif
