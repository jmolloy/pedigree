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
#ifndef RAWFS_DIRECTORY_H
#define RAWFS_DIRECTORY_H

#include <vfs/Directory.h>
#include <utilities/String.h>

class RawFsDir : public Directory
{
private:
    /** Copy constructors are hidden - unused! */
    RawFsDir(const RawFsDir &file);
    RawFsDir& operator =(const RawFsDir&);
public:
    /** Constructor, should only be called by RawFs. */
    RawFsDir(String name, class RawFs *pFs, File *pParent);
    ~RawFsDir() {}
    /** Reads directory contents into File* cache. */
    virtual void cacheDirectoryContents() {}

    virtual void fileAttributeChanged() {}

    virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer) {return 0;}
    virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer) {return 0;}


    /** Adds an entry to this directory.
	\note To be called by RawFS only. */
    void addEntry(File *pEntry);
    
    void removeRecursive();
};

#endif
