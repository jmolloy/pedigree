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

#include <Log.h>
#include <Module.h>
#include <processor/Processor.h>
#include <syscallError.h>
#include <utilities/List.h>
#include <utilities/StaticString.h>
#include <utilities/assert.h>
#include <utilities/utility.h>
#include <vfs/VFS.h>

#include "fat.h"
#include "FatFile.h"
#include "FatFilesystem.h"
#include "FatDirectory.h"

// helper functions

bool isPowerOf2(uint32_t n)
{
    uint8_t log;

    for (log = 0; log < 16; log++)
    {
        if (n & 1)
        {
            n >>= 1;
            return (n != 0) ? false : true;
        }
        n >>= 1;
    }
    return false;
}

FatFilesystem::FatFilesystem() :
        m_Superblock(), m_Superblock16(), m_Superblock32(), m_FsInfo(), m_Type(FAT12), m_DataAreaStart(0),
        m_RootDirCount(0), m_FatSector(0), m_RootDir(), m_BlockSize(0), m_pFatCache(0), m_FatLock(), m_pRoot(0),
        m_FatCache(), m_FreeClusterHint()
{
}

FatFilesystem::~FatFilesystem()
{
    if(m_pRoot)
        delete m_pRoot;
    if(m_pFatCache)
        delete [] m_pFatCache;
}

bool FatFilesystem::initialise(Disk *pDisk)
{
    m_pDisk = pDisk;

    // Attempt to read the superblock.
    uint8_t *buffer = reinterpret_cast<uint8_t*>(m_pDisk->read(0));

    memcpy(reinterpret_cast<void*> (&m_Superblock), reinterpret_cast<void*> (buffer), sizeof(Superblock));

    /** Validate the BPB and check for FAT FS */
    String devName;
    pDisk->getName(devName);

    /* check for EITHER a near jmp, or a jmp and a nop */
    if (m_Superblock.BS_jmpBoot[0] != 0xE9)
    {
        if (!(m_Superblock.BS_jmpBoot[0] == 0xEB && m_Superblock.BS_jmpBoot[2] == 0x90))
        {
            ERROR("FAT: Superblock not found on device " << devName << " [" << m_Superblock.BS_jmpBoot[0] << ", " << m_Superblock.BS_jmpBoot[2] << "]");
            return false;
        }
    }

    /** Check the FAT FS itself, ensuring it's valid */

    // SecPerClus must be a power of 2
    if (!isPowerOf2(m_Superblock.BPB_SecPerClus))
    {
        ERROR("FAT: SecPerClus not a power of 2 (" << m_Superblock.BPB_SecPerClus << ")");
        return false;
    }

    // and there must be at least 1 FAT, and at most 2
    if (m_Superblock.BPB_NumFATs < 1 || m_Superblock.BPB_NumFATs > 2)
    {
        ERROR("FAT: Too many (or too few) FATs (" << m_Superblock.BPB_NumFATs << ")");
        return false;
    }

    /** Start loading actual FS info */

    // load the 12/16/32 additional info structures (only one is actually VALID, but both are loaded nonetheless)
    memcpy(reinterpret_cast<void*> (&m_Superblock16), reinterpret_cast<void*> (buffer + 36), sizeof(Superblock16));
    memcpy(reinterpret_cast<void*> (&m_Superblock32), reinterpret_cast<void*> (buffer + 36), sizeof(Superblock32));

    // number of root directory sectors
    if (!m_Superblock.BPB_BytsPerSec) // we sanity check the value, because we divide by this later
    {
        return false;
    }
    uint32_t rootDirSectors = ((m_Superblock.BPB_RootEntCnt * 32) + (m_Superblock.BPB_BytsPerSec - 1)) / m_Superblock.BPB_BytsPerSec;

    // determine the size of the FAT
    uint32_t fatSz = (m_Superblock.BPB_FATSz16) ? m_Superblock.BPB_FATSz16 : m_Superblock32.BPB_FATSz32;

    // find the first data sector
    uint32_t firstDataSector = m_Superblock.BPB_RsvdSecCnt + (m_Superblock.BPB_NumFATs * fatSz) + rootDirSectors;

    // determine the number of data sectors, so we can determine FAT type
    uint32_t totDataSec = 0, totSec = (m_Superblock.BPB_TotSec16) ? m_Superblock.BPB_TotSec16 : m_Superblock.BPB_TotSec32;
    totDataSec = totSec - firstDataSector;

    if (!m_Superblock.BPB_SecPerClus) // again, sanity checking due to division by this
    {
        ERROR("FAT: SecPerClus is zero!");
        return false;
    }
    uint32_t clusterCount = totDataSec / m_Superblock.BPB_SecPerClus;

    // TODO: magic numbers here, perhaps #define MAXCLUS_{12|16|32} would work better for readability
    if (clusterCount < 4085)
    {
        m_Type = FAT12;
        NOTICE("FAT: Device " << devName << " is type FAT12");
    }
    else if (clusterCount < 65525)
    {
        m_Type = FAT16;
        NOTICE("FAT: Device " << devName << " is type FAT16");
    }
    else
    {
        m_Type = FAT32;
        NOTICE("FAT: Device " << devName << " is type FAT32");
    }

    switch (m_Type)
    {
    case FAT12:
    case FAT16:

        m_RootDir.sector = m_Superblock.BPB_RsvdSecCnt + (m_Superblock.BPB_NumFATs * fatSz);

        break;

    case FAT32:

        m_RootDir.cluster = m_Superblock32.BPB_RootClus;

        break;
    }

    // fill the filesystem data
    m_DataAreaStart = firstDataSector;
    m_RootDirCount = rootDirSectors;
    m_BlockSize = m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec;

    // read in the FAT32 FSInfo structure
    if (m_Type == FAT32)
    {
        uint32_t sec = m_Superblock32.BPB_FsInfo;
        readSectorBlock(sec, 512, reinterpret_cast<uintptr_t>(&m_FsInfo));
    }

    // determine the size of the FAT
    fatSz = (m_Superblock.BPB_FATSz16) ? m_Superblock.BPB_FATSz16 : m_Superblock32.BPB_FATSz32;
    fatSz *= m_Superblock.BPB_BytsPerSec;

    // read the FAT into cache
    m_FatSector = m_Superblock.BPB_RsvdSecCnt;

    //m_pFatCache = new uint8_t[fatSz];
    //readSectorBlock(fatSector, fatSz, reinterpret_cast<uintptr_t>(m_pFatCache));

    // Setup the free cluster hint for non-FAT32 volumes
    m_FreeClusterHint = 2;

    // Define the root directory early
    getRoot();

    return true;
}

Filesystem *FatFilesystem::probe(Disk *pDisk)
{
    FatFilesystem *pFs = new FatFilesystem();
    if (!pFs->initialise(pDisk))
    {
        delete pFs;
        return 0;
    }
    else
    {
        return pFs;
    }
}

File* FatFilesystem::getRoot()
{
    // needs to return a file referring to the root directory
    uint32_t cluster = 0;
    if (m_Type == FAT32)
        cluster = m_RootDir.cluster;

    FatFileInfo info;
    info.creationTime = info.modifiedTime = info.accessedTime = 0;

    if (!m_pRoot)
        m_pRoot = new FatDirectory(String(""), cluster, this, 0, info);

    return m_pRoot;
}

String FatFilesystem::getVolumeLabel()
{
    // The root directory (typically) contains the volume label, with a specific flag
    // In my experience, it's always the first entry, and it's always there. Even so,
    // we want to cater to unusual formats.
    //
    // In order to do so we check the entire root directory.

    uint32_t sz = m_BlockSize;

    uint32_t clus = 0;
    if (m_Type == FAT32)
        clus = m_RootDir.cluster;

    String volid;

    uint8_t* buffer = reinterpret_cast<uint8_t*>(readDirectoryPortion(clus));

    size_t i;
    bool endOfDir = false;
    while (true)
    {
        for (i = 0; i < sz; i += sizeof(Dir))
        {
            Dir* ent = reinterpret_cast<Dir*>(&buffer[i]);

            if (ent->DIR_Name[0] == 0)
            {
                endOfDir = true;
                break;
            }

            if (ent->DIR_Attr & ATTR_VOLUME_ID)
            {
                volid = convertFilenameFrom(String(reinterpret_cast<const char*>(ent->DIR_Name)));
                delete [] buffer;
                return volid;
            }
        }

        if (endOfDir)
            break;

        if (clus == 0 && m_Type != FAT32)
            break; // not found

        // find the next cluster in the chain, if this is the end, break, if not, continue
        clus = getClusterEntry(clus);
        if (clus == 0)
            break; // something broke!

        if (isEof(clus))
            break;

        // continue by reading in this cluster
        readCluster(clus, reinterpret_cast<uintptr_t> (buffer));

    }

    delete [] buffer;

    // none found, do a default
    NormalStaticString str;
    str += "no-volume-label@";
    str.append(reinterpret_cast<uintptr_t>(this), 16);
    return String(static_cast<const char*>(str));
}

/////////////////////////////////////////////////////////////////////////////

uint64_t FatFilesystem::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    // Sanity check.
    if (pFile->isDirectory())
        return 0;

    // the inode of the file is the first cluster
    uint32_t clus = pFile->getInode();
    if (clus == 0)
        return 0; // can't do it

    // validity checking
    if (static_cast<size_t>(location) >= pFile->getSize())
    {
        WARNING("FAT: Attempting to read past the EOF [loc=" << location << ", sz=" << size << ", fsz=" << pFile->getSize() << "]");
        return 0;
    }

    uint64_t endOffset = location + size;
    uint64_t finalSize = size;
    if (static_cast<size_t>(endOffset) > pFile->getSize())
    {
        finalSize = pFile->getSize() - location;

        // overflow (location > size) or zero bytes required (location == size)
        if ((finalSize == 0) || (finalSize > pFile->getSize()))
        {
            WARNING("FAT: location + size > EOF");
            return 0;
        }
    }

    // finalSize holds the total amount of data to read, now find the cluster and sector offsets
    uint32_t clusOffset = location / (m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec);
    uint32_t firstOffset = location % (m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec); // the offset within the cluster specified above to start reading from

    // tracking info

    uint64_t bytesRead = 0;
    uint64_t currOffset = firstOffset;
    while (clusOffset)
    {
        clus = getClusterEntry(clus);
        if (clus == 0 || isEof(clus))
        {
            WARNING("FAT: CLUSTER FAIL - " << clus << ", cluster offset = " << clusOffset << "."); // <-- This is where the installer + lodisk is failing.
            WARNING("    -> file: " << pFile->getFullPath());
            WARNING("    -> size: " << pFile->getSize());
            return 0; // can't do it
        }
        clusOffset--;
    }

    // buffers
    uint8_t* tmpBuffer = new uint8_t[m_BlockSize];
    uint8_t* destBuffer = reinterpret_cast<uint8_t*>(buffer);

    // main read loop
    while (true)
    {
        // read in the entire cluster
        readCluster(clus, reinterpret_cast<uintptr_t> (tmpBuffer));

        // How many bytes should we copy?
        size_t bytesToCopy = finalSize - bytesRead;
        if(bytesToCopy > m_BlockSize)
        {
            bytesToCopy = m_BlockSize;
        }

        // Perform the copy.
        memcpy(&destBuffer[bytesRead], &tmpBuffer[currOffset], bytesToCopy);
        currOffset += bytesToCopy;
        bytesRead += bytesToCopy;

        // Done?
        if(bytesRead == finalSize)
        {
            delete [] tmpBuffer;
            return bytesRead;
        }

        // end of cluster, set the offset back to zero
        currOffset = 0;

        // grab the next cluster, check for EOF
        clus = getClusterEntry(clus);
        if (clus == 0)
            break; // something broke!

        if (isEof(clus))
            break;
    }

    delete [] tmpBuffer;

    // if we reach here, something's gone wrong
    WARNING("FAT: read returning zero... Something's not right.");
    return 0;
}

/////////////////////////////////////////////////////////////////////////////

uint32_t FatFilesystem::findFreeCluster(bool bLock)
{
    size_t j;
    uint32_t clus;
    uint32_t totalSectors = m_Superblock.BPB_TotSec32;
    if (totalSectors == 0)
    {
        if (m_Type != FAT32)
            totalSectors = m_Superblock.BPB_TotSec16;
        else
        {
            return 0;
        }
    }

    uint32_t mask = m_Type == FAT32 ? 0x0FFFFFFF : 0xFFFF;

    for (j = (m_Type == FAT32 ? m_FsInfo.FSI_NxtFree : m_FreeClusterHint); j < (totalSectors / m_Superblock.BPB_SecPerClus); j++)
    {
        clus = getClusterEntry(j, false);
        if ((clus & mask) == 0)
        {
            /// \todo For FAT32, update the FSInfo structure
            setClusterEntry(j, eofValue(), false); // default to it being EOF - ie, pin the cluster

            // All done!
            m_FreeClusterHint = j + 1;
            return j;
        }
    }

    FATAL("findFreeCluster returning zero!");
    return 0;
}

/////////////////////////////////////////////////////////////////////////////

uint64_t FatFilesystem::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    NOTICE("FatFilesystem::write");

    // test whether the entire Filesystem is read-only.
    if (m_bReadOnly)
    {
        SYSCALL_ERROR(ReadOnlyFilesystem);
        return 0;
    }

    // we do so much work with the FAT here that locking is a necessity
    // LockGuard<Mutex> guard(m_FatLock);

    int64_t fileSizeChange = 0;
    if ((location + size) > pFile->getSize())
        fileSizeChange = (location + size) - pFile->getSize();

    uint32_t firstClus = pFile->getInode();

    if (firstClus == 0)
    {
        // find a free cluster for this file
        uint32_t freeClus = findFreeCluster();
        if (freeClus == 0)
        {
            SYSCALL_ERROR(NoSpaceLeftOnDevice);
            return 0;
        }

        // set EOF
        setClusterEntry(freeClus, eofValue(), false);
        firstClus = freeClus;

        // write into the directory entry, and into the File itself
        pFile->setInode(freeClus);
        setCluster(pFile, freeClus);
    }

    uint32_t clusSize = m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec;
    uint32_t finalOffset = location + size;
    uint32_t offsetSector = location / m_Superblock.BPB_BytsPerSec;
    uint32_t clus = 0;

    // does the file currently have enough clusters to allow us to write without stopping?
    int i = clusSize;
    int j = pFile->getSize() / i;
    if (pFile->getSize() % i)
        j++; // extra cluster (integer division)
    if (j == 0)
        j = 1; // always one cluster

    uint32_t finalCluster = j * i;
    uint32_t numExtraBytes = 0;

    // if the final offset is past what we already have in the cluster chain, fill in the blanks
    if (finalOffset > finalCluster)
    {
        numExtraBytes = finalOffset - finalCluster;

        j = numExtraBytes / i;
        if (numExtraBytes % i)
            j++;

        clus = firstClus;

        uint32_t lastClus = clus;
        while (!isEof(clus))
        {
            lastClus = clus;
            clus = getClusterEntry(clus, false);
        }

        uint32_t prev = 0;
        for (i = 0; i < j; i++)
        {
            prev = lastClus;
            lastClus = findFreeCluster();
            if (!lastClus)
            {
                SYSCALL_ERROR(NoSpaceLeftOnDevice);
                return 0;
            }

            setClusterEntry(prev, lastClus, false);
        }

        setClusterEntry(lastClus, eofValue(), false);
    }

    uint64_t finalSize = size;

    // finalSize holds the total amount of data to read, now find the cluster and sector offsets
    uint32_t clusOffset = offsetSector / m_Superblock.BPB_SecPerClus; //location / (m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec);
    uint32_t firstOffset = location % (m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec); // the offset within the cluster specified above to start reading from

    // tracking info

    uint64_t bytesWritten = 0;
    uint64_t currOffset = firstOffset;
    clus = firstClus;
    for (uint32_t z = 0; z < clusOffset; z++)
    {
        clus = getClusterEntry(clus, false);
        if (clus == 0 || isEof(clus))
            return 0;
    }

    // buffers
    uint8_t* tmpBuffer = new uint8_t[m_BlockSize];
    uint8_t* srcBuffer = reinterpret_cast<uint8_t*>(buffer);

    // main write loop
    while (true)
    {
        // read in the entire cluster
        readCluster(clus, reinterpret_cast<uintptr_t> (tmpBuffer));

        // read...
        while (currOffset < m_BlockSize)
        {
            tmpBuffer[currOffset] = srcBuffer[bytesWritten];

            currOffset++;
            bytesWritten++;

            if (bytesWritten == finalSize)
            {
                // update the size on disk, if needed
                if (fileSizeChange != 0)
                {
                    updateFileSize(pFile, fileSizeChange);
                    pFile->setSize(pFile->getSize() + fileSizeChange);
                }

                // and now actually write the updated file contents
                writeCluster(clus, reinterpret_cast<uintptr_t> (tmpBuffer));

                delete [] tmpBuffer;
                return bytesWritten;
            }
        }

        writeCluster(clus, reinterpret_cast<uintptr_t> (tmpBuffer));

        // end of cluster, set the offset back to zero
        currOffset = 0;

        // grab the next cluster, check for EOF
        clus = getClusterEntry(clus, false);
        if (clus == 0)
            break; // something broke!

        if (isEof(clus))
            break;
    }

    delete [] tmpBuffer;

    return bytesWritten;
}

/////////////////////////////////////////////////////////////////////////////

void FatFilesystem::updateFileSize(File* pFile, int64_t sizeChange)
{
    // don't bother reading the directory if there's no actual change
    if (sizeChange == 0)
        return;

    uint32_t dirClus = static_cast<FatFile*>(pFile)->getDirCluster();
    uint32_t dirOffset = static_cast<FatFile*>(pFile)->getDirOffset();

    Dir* p = getDirectoryEntry(dirClus, dirOffset);
    if (!p)
        return;
    p->DIR_FileSize += sizeChange;
    writeDirectoryEntry(p, dirClus, dirOffset);

    delete p;
}

void FatFilesystem::setCluster(File* pFile, uint32_t clus)
{
    // don't bother reading and writing if the cluster is zero
    if (clus == 0)
        return;

    uint32_t dirClus = static_cast<FatFile*>(pFile)->getDirCluster();
    uint32_t dirOffset = static_cast<FatFile*>(pFile)->getDirOffset();

    Dir* p = getDirectoryEntry(dirClus, dirOffset);
    if (!p)
        return;
    p->DIR_FstClusLO = clus & 0xFFFF;
    p->DIR_FstClusHI = (clus >> 16) & 0xFFFF;
    writeDirectoryEntry(p, dirClus, dirOffset);

    delete p;
}

void* FatFilesystem::readDirectoryPortion(uint32_t clus)
{
    uint32_t dirClus = clus;
    uint8_t* dirBuffer = 0;

    bool secMethod = false;
    if (dirClus == 0)
    {
        if (m_Type != FAT32)
        {
            uint32_t sec = m_RootDir.sector;
            uint32_t sz = m_RootDirCount * m_Superblock.BPB_BytsPerSec;

            dirBuffer = new uint8_t[sz];
            readSectorBlock(sec, sz, reinterpret_cast<uintptr_t>(dirBuffer));

            secMethod = true;
        }
        else
            return 0;
    }
    else
    {
        dirBuffer = new uint8_t[m_BlockSize];
        readCluster(dirClus, reinterpret_cast<uintptr_t>(dirBuffer));
    }

    return reinterpret_cast<void*>(dirBuffer);
}

void FatFilesystem::writeDirectoryPortion(uint32_t clus, void* p)
{
    if (!p)
        return;
    bool secMethod = false;
    uint32_t sz = m_BlockSize;
    uint32_t sec = m_RootDir.sector;
    if (clus == 0)
    {
        if (m_Type != FAT32)
        {
            sz = m_RootDirCount * m_Superblock.BPB_BytsPerSec;
            secMethod = true;
        }
        else
            return;
    }

    if (secMethod)
        writeSectorBlock(sec, sz, reinterpret_cast<uintptr_t>(p));
    else
        writeCluster(clus, reinterpret_cast<uintptr_t>(p));
}

Dir* FatFilesystem::getDirectoryEntry(uint32_t clus, uint32_t offset)
{
    uint8_t* dirBuffer = reinterpret_cast<uint8_t*>(readDirectoryPortion(clus));
    if (!dirBuffer)
        return 0;

    Dir* ent = reinterpret_cast<Dir*>(&dirBuffer[offset]);
    Dir* ret = new Dir;
    memcpy(ret, ent, sizeof(Dir));

    delete [] dirBuffer;

    return ret;
}

void FatFilesystem::writeDirectoryEntry(Dir* dir, uint32_t clus, uint32_t offset)
{
    // don't bother reading and writing if the cluster is zero or if there's no
    // entry to write
    if (dir == 0)
        return;

    uint8_t* dirBuffer = reinterpret_cast<uint8_t*>(readDirectoryPortion(clus));
    if (!dirBuffer)
        return;

    Dir* ent = reinterpret_cast<Dir*>(&dirBuffer[offset]);
    memcpy(ent, dir, sizeof(Dir));

    writeDirectoryPortion(clus, dirBuffer);

    delete [] dirBuffer;
}

void FatFilesystem::fileAttributeChanged(File *pFile)
{
}

void FatFilesystem::cacheDirectoryContents(File *pFile)
{
}

bool FatFilesystem::readCluster(uint32_t block, uintptr_t buffer)
{
    block = getSectorNumber(block);
    readSectorBlock(block, m_BlockSize, buffer);
    return true;
}

bool FatFilesystem::readSectorBlock(uint32_t sec, size_t size, uintptr_t buffer)
{
    assert(buffer);
    size_t off = 0;
    while (size)
    {
        size_t sz = (size > 512) ? 512 : size;
        uintptr_t buff = m_pDisk->read(
            (static_cast<uint64_t>(m_Superblock.BPB_BytsPerSec) * static_cast<uint64_t>(sec)) + off);
        if(!buff)
            return false;
        memcpy(reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(buff),
               sz);
        buffer += sz;
        size -= sz;
        off += sz;
    }
    return true;
}

bool FatFilesystem::writeCluster(uint32_t block, uintptr_t buffer)
{
    block = getSectorNumber(block);
    writeSectorBlock(block, m_BlockSize, buffer);
    return true;
}

bool FatFilesystem::writeSectorBlock(uint32_t sec, size_t size, uintptr_t buffer)
{
    assert(buffer);
    size_t off = 0;
    while (size)
    {
        size_t sz = (size > 4096) ? 4096 : size;
        uintptr_t buff = m_pDisk->read(static_cast<uint64_t>(m_Superblock.BPB_BytsPerSec)*static_cast<uint64_t>(sec)+off);
        memcpy(reinterpret_cast<void*>(buff), reinterpret_cast<void*>(buffer),
               sz);
        buffer += sz;
        size -= sz;
        off += sz;
    }
    return true;
}

uint32_t FatFilesystem::getSectorNumber(uint32_t cluster)
{
    return ((cluster - 2) * m_Superblock.BPB_SecPerClus) + m_DataAreaStart;
}

uint32_t FatFilesystem::getClusterEntry(uint32_t cluster, bool bLock)
{
    uint32_t fatOffset = 0;
    switch (m_Type)
    {
        case FAT12:
            fatOffset = cluster + (cluster / 2);
            break;

        case FAT16:
            fatOffset = cluster * 2;
            break;

        case FAT32:
            fatOffset = cluster * 4;
            break;
    }

    //uint8_t *fatBlocks = new uint8_t[m_Superblock.BPB_BytsPerSec * 2];

    // Reading from the FAT - critical section
    while(!m_FatLock.enter())
    {
        Scheduler::instance().yield();
    }
    uint8_t *fatBlocks = reinterpret_cast<uint8_t*>(m_FatCache.lookup(fatOffset / m_Superblock.BPB_BytsPerSec));
    m_FatLock.leave();

    if(fatBlocks && (m_Type == FAT12))
    {
        FATAL("Oooer missus, work needed heres");
        //fatBlocks = reinterpret_cast<uint8_t*>(m_FatCache.lookup((fatOffset / m_Superblock.BPB_BytsPerSec) + 1, fatBlocks + m_Superblock.BPB_BytsPerSec));
    }
    if(!fatBlocks)
    {
        m_FatLock.acquire();

        fatBlocks = new uint8_t[m_Superblock.BPB_BytsPerSec * 2];
        if(!readSectorBlock(m_FatSector + (fatOffset / m_Superblock.BPB_BytsPerSec), m_Superblock.BPB_BytsPerSec * 2, reinterpret_cast<uintptr_t>(fatBlocks)))
        {
            ERROR("FAT: getClusterEntry: reading from the FAT failed");
            //delete [] fatBlocks;
            m_FatLock.release();
            return 0;
        }

        m_FatCache.insert(fatOffset / m_Superblock.BPB_BytsPerSec, reinterpret_cast<uintptr_t>(fatBlocks));
        m_FatCache.insert((fatOffset / m_Superblock.BPB_BytsPerSec) + 1, reinterpret_cast<uintptr_t>(fatBlocks + m_Superblock.BPB_BytsPerSec));

        m_FatLock.release();
    }

    // read from cache
    fatOffset %= m_Superblock.BPB_BytsPerSec;
    uint32_t fatEntry = * reinterpret_cast<uint32_t*> (&fatBlocks[fatOffset]);

    /// \todo
    //delete [] fatBlocks;

    // calculate
    uint32_t ret = 0;
    switch (m_Type)
    {
        case FAT12:
            ret = fatEntry;

            // FAT12 entries are 1.5 bytes
            if (cluster & 0x1)
                ret >>= 4;
            else
                ret &= 0x0FFF;
            ret &= 0xFFFF;

            break;

        case FAT16:

            ret = fatEntry;
            ret &= 0xFFFF;

            break;

        case FAT32:

            ret = fatEntry & 0x0FFFFFFF;

            break;
    }

    return ret;
}

uint32_t FatFilesystem::setClusterEntry(uint32_t cluster, uint32_t value, bool bLock)
{
    if (cluster == 0)
    {
        FATAL("setClusterEntry called with invalid arguments - " << cluster << "/" << value << "!");
        return 0;
    }

    uint32_t fatOffset = 0;
    switch (m_Type)
    {
        case FAT12:
            fatOffset = cluster + (cluster / 2);
            break;

        case FAT16:
            fatOffset = cluster * 2;
            break;

        case FAT32:
            fatOffset = cluster * 4;
            break;
    }

    uint32_t ent = getClusterEntry(cluster, bLock);

//    uint8_t *fatBlocks = new uint8_t[m_Superblock.BPB_BytsPerSec * 2];


    uint8_t *fatBlocks = reinterpret_cast<uint8_t*>(m_FatCache.lookup(fatOffset / m_Superblock.BPB_BytsPerSec));
    if(fatBlocks && (m_Type == FAT12))
    {
        FATAL("Ooer missus, work needed here");
//        fatBlocks = reinterpret_cast<uint8_t*>(m_FatCache.lookup((fatOffset / m_Superblock.BPB_BytsPerSec) + 1, fatBlocks + m_Superblock.BPB_BytsPerSec));
    }
    if(!fatBlocks)
    {
        ERROR("FAT: setClusterEntry: getClusterEntry didn't read sectors from cache properly?");
        //delete [] fatBlocks;
        return 0;
    }

    uint32_t oldOffset = fatOffset;
    fatOffset %= m_Superblock.BPB_BytsPerSec;

    uint32_t origEnt = ent;
    uint32_t setEnt = value;

    // Calculate and write back into the cache
    switch (m_Type)
    {
        case FAT12:

            if (cluster & 0x1)
                setEnt >>= 4;
            else
                setEnt &= 0x0FFF;
            setEnt &= 0xFFFF;

            if (cluster & 0x1)
            {
                value <<= 4;
                origEnt &= 0x000F;
            }
            else
            {
                value &= 0x0FFF;
                origEnt &= 0xF000;
            }

            setEnt = origEnt | value;

            * reinterpret_cast<uint16_t*> (&fatBlocks[fatOffset]) = setEnt;

            break;

        case FAT16:

            setEnt = value;

            * reinterpret_cast<uint16_t*> (&fatBlocks[fatOffset]) = setEnt;

            break;

        case FAT32:

            value &= 0x0FFFFFFF;
            setEnt = origEnt & 0xF0000000;
            setEnt |= value;

            * reinterpret_cast<uint32_t*> (&fatBlocks[fatOffset]) = setEnt;

            break;
    }

    uint32_t fatSector = m_FatSector + (oldOffset / m_Superblock.BPB_BytsPerSec);

    // Grab the FAT lock - we're updating it now
    m_FatLock.acquire();

    // Write back to the FAT
    writeSectorBlock(fatSector, m_Superblock.BPB_BytsPerSec * 2, reinterpret_cast<uintptr_t>(fatBlocks));

    // Write back to the cache
    m_FatCache.insert(oldOffset / m_Superblock.BPB_BytsPerSec, reinterpret_cast<uintptr_t>(fatBlocks));
    if(m_Type == FAT12)
        m_FatCache.insert((oldOffset / m_Superblock.BPB_BytsPerSec) + 1, reinterpret_cast<uintptr_t>(fatBlocks + m_Superblock.BPB_BytsPerSec));

    // All done with the update
    m_FatLock.release();

    ///\todo
    //delete [] fatBlocks;

    // We're pedantic and as such we check things, but only if debugging
#if defined(DEBUGGER) && defined(ADDITIONAL_CHECKS)
    uint32_t val = getClusterEntry(cluster, false);
    if (val != value)
        FATAL("setClusterEntry has failed on cluster " << cluster << ": " << val << "/" << value << ".");
#endif

    return setEnt;
}

String FatFilesystem::convertFilenameTo(String filename)
{
    static NormalStaticString ret;
    ret.clear();

    // Special dot & dotdot handling. Because periods are eaten by the algorithm, we need to
    // ensure that the dot and dotdot entries are returned with only padding.
    if (!strcmp(static_cast<const char*>(filename), ".") || !strcmp(static_cast<const char*>(filename), ".."))
    {
        ret = filename;
    }
    else
    {
        size_t i;
        bool bPeriod = false;
        for (i = 0; i < 11; i++)
        {
            if (i >= filename.length())
                break;
            if (filename[i] != '.')
                ret += toUpper(filename[i]);
            else
            {
                bPeriod = true;
                i++;
                break;
            }
        }

        if (bPeriod)
        {
            ret.pad(8);

            size_t j = 0;
            size_t nChars = 0;
            while (((i + j) < filename.length()) && (nChars < 3))
            {
                ret += toUpper(filename[i + j]);
                nChars++;
                j++;
            }
        }
    }

    // And finally, pad out whatever we need to to finish off
    ret.pad(11);

    ret += '\0';
    return String(static_cast<const char*>(ret));
}

String FatFilesystem::convertFilenameFrom(String filename)
{
    static NormalStaticString ret;
    ret.clear();

    size_t i;
    for (i = 0; i < 8; i++)
    {
        if (i >= filename.length())
            break;
        if (filename[i] != ' ')
            ret += toLower(filename[i]);
        else
            break;
    }

    for (i = 0; i < 3; i++)
    {
        if ((8 + i) >= filename.length())
            break;
        if (filename[8 + i] != ' ')
        {
            if (i == 0)
                ret += '.';
            ret += toLower(filename[8 + i]);
        }
        else
            break;
    }

    ret += '\0';

    return String(static_cast<const char*>(ret));
}

void FatFilesystem::truncate(File *pFile)
{
    NOTICE("FatFilesystem::truncate");

    // First of all, set the file size to zero, so that if the file is used
    // elsewhere it's updated.
    updateFileSize(pFile, -pFile->getSize());
    pFile->setSize(0);

    // And then clean up its cluster chain so we only have one remaining
    // Then, clean up the cluster chain
    uint32_t clus = pFile->getInode(), prev = 0;
    if (clus != 0)
    {
        prev = clus;
        clus = getClusterEntry(clus, true);
        setClusterEntry(prev, eofValue(), true);

        // If the second cluster is not EOF, clean up the chain
        if(!isEof(clus))
        {
            while(!isEof(clus))
            {
                prev = clus;
                clus = getClusterEntry(clus, true);
                setClusterEntry(prev, 0, true);
            }
            setClusterEntry(prev, 0, true);
        }
    }
}

void FatFilesystem::extend(File *pFile, size_t size)
{
    // The File object still has the old size until after we return.
    if(pFile->getSize() >= size)
    {
        // Don't extend - no need.
        return;
    }

    uint32_t firstClus = pFile->getInode();
    int64_t sizeChange = size - pFile->getSize();

    size_t clusSize = m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec;

    // Find a free cluster for the file if none exists yet.
    if (firstClus == 0)
    {
        // Get an available free cluster.
        uint32_t freeClus = findFreeCluster();
        if (freeClus == 0)
        {
            SYSCALL_ERROR(NoSpaceLeftOnDevice);
            return;
        }

        // This cluster is now EOF (first cluster of the file we're linking in)
        setClusterEntry(freeClus, eofValue(), false);
        firstClus = freeClus;

        // Update the cluster and file object.
        pFile->setInode(freeClus);
        setCluster(pFile, freeClus);

        // Do we need to do anything more?
        if(clusSize >= size)
        {
            return;
        }
    }

    uint32_t finalOffset = size;
    uint32_t offsetSector = pFile->getSize() / m_Superblock.BPB_BytsPerSec;
    uint32_t clus = 0;

    // Figure out how many (if any) additional clusters we need to link in now.
    int i = clusSize;
    int j = pFile->getSize() / i;
    if (pFile->getSize() % i)
        j++; // extra cluster (integer division)
    if (j == 0)
        j = 1; // always one cluster

    uint32_t finalCluster = j * i;
    uint32_t numExtraBytes = 0;

    // Do we need to link in extra clusters?
    if (finalOffset > finalCluster)
    {
        numExtraBytes = finalOffset - finalCluster;

        j = numExtraBytes / i;
        if (numExtraBytes % i)
            j++;

        clus = firstClus;

        uint32_t lastClus = clus;
        while (!isEof(clus))
        {
            lastClus = clus;
            clus = getClusterEntry(clus, false);
        }

        uint32_t prev = 0;
        for (i = 0; i < j; i++)
        {
            prev = lastClus;
            lastClus = findFreeCluster();
            if (!lastClus)
            {
                SYSCALL_ERROR(NoSpaceLeftOnDevice);
                return;
            }

            setClusterEntry(prev, lastClus, false);
        }

        // Final cluster must always point to EOF.
        setClusterEntry(lastClus, eofValue(), false);
    }

    // Update the directory now that we are done with the FAT.
    updateFileSize(pFile, sizeChange);
}

File *FatFilesystem::createFile(File *parentDir, String filename, uint32_t mask, bool bDirectory, uint32_t dirClus)
{
    // Validate input
    if(!parentDir->isDirectory())
    {
      return 0;
    }

    FatFileInfo info;
    info.creationTime = 0;
    info.modifiedTime = 0;
    info.accessedTime = 0;

    // Directory or File?
    // Note that new files in FAT always have a zero cluster, but new directories
    // require a cluster (to keep the "." and ".." entries from jumping in)
    File *pFile;
    if (bDirectory)
        pFile = new FatDirectory(filename, dirClus, this, parentDir, info);
    else
    {
        // Deviation from the spec here: Because the 'inode' is used for fstat,
        // we can't leave it at zero or else all newly created files without
        // data will look the same!
        uint32_t clus = findFreeCluster();
        setClusterEntry(clus, eofValue());
        pFile = new FatFile(
            filename,
            0,
            0,
            0,
            clus,
            this,
            0,
            0xdeadbeef, // Sentinel values that'll throw an error if they're used
            0xbeefdead, // before being set to correct values.
            parentDir
        );
    }

    FatDirectory *parent = static_cast<FatDirectory *>(Directory::fromFile(parentDir));
    if (!parent->addEntry(filename, pFile, (bDirectory ? 1 : 0)))
    {
        delete pFile;
        return 0;
    }

    return pFile;
}

bool FatFilesystem::createFile(File *parent, String filename, uint32_t mask)
{
    File *f = createFile(parent, filename, mask, false);
    return (f != 0);
}

bool FatFilesystem::createDirectory(File* parent, String filename)
{
    // Allocate a cluster for the directory itself
    uint32_t clus = findFreeCluster(true);
    if (!clus)
        return false;

    File* f = createFile(parent, filename, 0, true, clus);
    if (!f)
    {
        setClusterEntry(clus, 0);
        return false;
    }

    FatDirectory *fatDir = static_cast<FatDirectory *>(f);
    setClusterEntry(clus, eofValue());
    fatDir->setInode(clus);
    setCluster(f, clus);

    File* dot = createFile(f, String("."), 0, true, clus);
    File* dotdot = createFile(f, String(".."), 0, true, parent->getInode());

    if (!dot || !dotdot)
    {
        // If either is valid, remove it from the directory, then remove ourselves
        if (dot)
            remove(f, dot);
        if (dotdot)
            remove(f, dotdot);
        remove(parent, f);
        delete f;
        return false;
    }

    setCluster(dot, dot->getInode());
    setCluster(dotdot, dotdot->getInode());

    return true;
}

bool FatFilesystem::createSymlink(File* parent, String filename, String value)
{
    /// \todo Figure out a way to do FAT symlinks - per-directory table perhaps?
    return false;
}

bool FatFilesystem::remove(File* parent, File* file)
{
    FatDirectory *parentDir = static_cast<FatDirectory *>(Directory::fromFile(parent));

    // Firstly, remove from the directory itself
    if (!parentDir->removeEntry(file))
        return false;

    //LockGuard<Mutex> guard(m_FatLock);

    // Then, clean up the cluster chain
    uint32_t clus = file->getInode();
    if (clus != 0)
    {
        uint32_t prev = 0;
        while (true)
        {
            prev = clus;
            clus = getClusterEntry(clus, false);
            setClusterEntry(prev, 0, false);

            if (clus == 0)
            {
                ERROR("Found a zero cluster during FatFilesystem::remove...");
                break;
            }

            if (isEof(clus))
                break;
        }
    }

    return true;
}

static void initFat()
{
    VFS::instance().addProbeCallback(&FatFilesystem::probe);
}

static void destroyFat()
{
}

MODULE_INFO("fat", &initFat, &destroyFat, "vfs");
