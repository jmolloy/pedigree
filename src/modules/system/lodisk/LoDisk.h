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
#ifndef LODISK_H
#define LODISK_H

#include <processor/types.h>
#include <utilities/String.h>
#include <vfs/File.h>
#include <vfs/VFS.h>
#include <machine/Disk.h>
#include <utilities/Cache.h>
#include <processor/MemoryRegion.h>

/// \todo Expose functionality here as a Service - specifically creating/removing disks.

/// 4 KB is enough to fit 8 HD sectors and 2 CD sectors, which makes it a good
/// choice for the page cache size.
#define FILEDISK_PAGE_SIZE      4096

/** Abstraction for Files as Disks
  * This allows disk images to be opened as a real disk for installer ramdisks
  * or for testing.
  *
  * Specifying RamOnly causes the file to be stored only in RAM with no changes
  * committed to the file.
  */
class FileDisk : public Disk
{
    public:

        enum AccessType
        {
            /** Read from file into RAM, changes persist in
              * RAM, never written to file
              */
            RamOnly = 0,

            /** Read from file, changes go to file */
            Standard
        };

        FileDisk() :
            m_pFile(0), m_Mode(Standard), m_Cache(),
            m_MemRegion("FileDisk-Failed"), m_ReqMutex(false)
        {
            FATAL("I require a constructor with arguments!");
        }

        FileDisk(String file, AccessType mode = Standard);
        virtual ~FileDisk();

        virtual void getName(String &str)
        {
            if(m_pFile)
                str = m_pFile->getName();
            else
                str = String("FileDisk");
        }

        bool initialise();

        virtual uintptr_t read(uint64_t location);
        virtual void write(uint64_t location);
        virtual void align(uint64_t location);

        /// None of our writes ever end up back on the loaded file.
        /// \todo Could it be possible to allow writes to go through to the file we've mounted?
        ///       Then this could return true if the backing device is read-only, false otherwise.
        virtual bool cacheIsCritical()
        {
            return true;
        }

    private:

        FileDisk(const FileDisk&);
        FileDisk& operator = (const FileDisk&);

        /// File we're using as a disk
        File *m_pFile;

        /// Access mode
        AccessType m_Mode;

        Cache m_Cache;

        // Our region of memory
        MemoryRegion m_MemRegion;

        // Mutex to ensure only one request runs at a time
        Mutex m_ReqMutex;

        uint64_t m_AlignPoints[8];
        size_t m_nAlignPoints;
};

#endif
