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
#ifndef MEMORY_MAPPED_FILE_H
#define MEMORY_MAPPED_FILE_H

#include "File.h"

#include <processor/PageFaultHandler.h>
#include <utilities/Tree.h>
#include <utilities/List.h>
#include <process/Mutex.h>
#include <process/Process.h>
#include <LockGuard.h>

/** \addtogroup vfs
    @{ */

/** \file
    \brief Memory-mapped file interface

    Provides a mechanism for mapping Files into the address space.
    
    \todo Handle writing of files, not just reading. */

class MemoryMappedFileManager;

/** This class allows a file to be mapped into memory at a specific or
    at a chosen location. It stores the V->P mappings so they can be
    reused across multiple address spaces.

    For caching purposes, a static factory function is provided for 
    acquisition and destruction. */
class MemoryMappedFile
{
public:
    /** Constructor - to be called from MemoryMappedFileManager only! */
    MemoryMappedFile(File *pFile, size_t extentOverride = 0, bool bShared = false);

    /** Secondary constructor for anonymous memory mapped "file". */
    MemoryMappedFile(size_t anonMapSize);

    /** Destructor */
    virtual ~MemoryMappedFile();

    /** Loads this file.
        \param address Load at the given address, or if address is 0, finds some free space and returns the start address in address. */
    bool load(uintptr_t &address, Process *pProcess=0, size_t extentOverride = 0);

    /** Unloads this file.
        \note Should ONLY be called from MemoryMappedFileManager! */
    void unload(uintptr_t address);

    /** Trap occurred. Should be called only from MemoryMappedFileManager!
        \param address The address of the fault.
        \param offset The starting offset of this mmappedfile in memory. */
    void trap(uintptr_t address, uintptr_t offset, uintptr_t fileoffset, bool bIsWrite);

    /** Mark this map for deletion when its reference count drops to zero - i.e. the underlying File has changed. */
    void markForDeletion()
    {m_bMarkedForDeletion = true;}

    /** Increase the reference count. */
    void increaseRefCount()
    {m_RefCount++;}

    /** Decrease the reference count.
        \return True if the object should now be deleted, false otherwise.*/
    bool decreaseRefCount()
    {
        m_RefCount--;
        if (m_RefCount == 0) return true;//m_bMarkedForDeletion;
        else return false;
    }

    size_t getExtent()
    {return m_Extent;}

    File *getFile()
    {
        return m_pFile;
    }
private:
    MemoryMappedFile(const MemoryMappedFile &);
    MemoryMappedFile &operator = (const MemoryMappedFile &);

    /** The file to map. */
    File *m_pFile;

    /** Map of V->P mappings. */
    Tree<uintptr_t, uintptr_t> m_Mappings;

    /** True if this file is marked for deletion - this will occur if
        the underlying file has changed since we were made. */
    bool m_bMarkedForDeletion;

    /** The size of the file from start to end, rounded (up) to the nearest
        page. */
    size_t m_Extent;

    /** Reference count. */
    size_t m_RefCount;

    Mutex m_Lock;

    /** Whether or not this mmap is shared (ie, data written is available to
     * other processes in the system).
     */
    bool m_bShared;
};

/** This class is a multiplexing trap handler, to take traps pertaining to
    MemoryMappedFiles and dispatch them. */
class MemoryMappedFileManager : public MemoryTrapHandler
{
public:
    /** Singleton instance */
    static MemoryMappedFileManager &instance()
    {return m_Instance;}

    /** Add a new mapping; Creates or gets from cache a new MemoryMappedFile. 
        \param pFile The VFS file to map.
        \param [out] address The address the file gets mapped to. 
        \return A pointer to a MemoryMappedFile object representing the mapped file. */
    MemoryMappedFile *map(File *pFile, uintptr_t &address, size_t sizeOverride = 0, size_t offset = 0, bool shared = true);

    /** Remove a specific mapping from this address space.
        \param pFile The MemoryMappedFile to remove the mapping for. */
    void unmap(MemoryMappedFile *pFile);

    /** Replicates the mappings in the current address space into the
        target Process' address space. The newly used regions are allocated
        from the target Process' memory allocator too.
        \param pTarget The process to clone into. */
    void clone(Process *pTarget);

    /** Removes all mappings from this address space. */
    void unmapAll();
    
    //
    // MemoryTrapHandler interface.
    //
    bool trap(uintptr_t address, bool bIsWrite);

private:
    /** Default and only constructor. Registers with PageFaultHandler. */
    MemoryMappedFileManager();
    ~MemoryMappedFileManager();

    /** Singleton instance. */
    static MemoryMappedFileManager m_Instance;

    /** A virtual address space can have multiple memory mapped files - 
        This is the type of a particular instantiation of a mmfile. */
    struct MmFile
    {
        MmFile(uintptr_t o, uintptr_t s, uintptr_t fo, MemoryMappedFile *f) :
            offset(o), size(s), fileoffset(fo), file(f)
        {}
        uintptr_t offset;
        uintptr_t size;
        uintptr_t fileoffset;
        MemoryMappedFile *file;
    };
    typedef List<MmFile*> MmFileList;
    
    /** Cache of virtual address spaces -> MmFileLists. */
    Tree<VirtualAddressSpace*, MmFileList*> m_MmFileLists;

    /** Global cache of valid files. */
    Tree<File*,MemoryMappedFile*> m_Cache;

    /** Lock for the cache. */
    Mutex m_CacheLock;
};

/** @} */

#endif
