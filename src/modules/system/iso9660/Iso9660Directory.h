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
#ifndef ISO9660DIRECTORY_H
#define ISO9660DIRECTORY_H

#include <vfs/Filesystem.h>
#include <vfs/Directory.h>
#include <utilities/List.h>
#include <utilities/Vector.h>
#include <utilities/Tree.h>
#include <process/Mutex.h>
#include <LockGuard.h>

#include "Iso9660Filesystem.h"
#include "iso9660.h"

class Iso9660Directory : public Directory
{
  friend class Iso9660File;

private:
  Iso9660Directory(const Iso9660Directory &);
  Iso9660Directory& operator =(const Iso9660Directory&);
public:
  Iso9660Directory(String name, size_t inode,
              class Iso9660Filesystem *pFs, File *pParent, Iso9660DirRecord &dirRec,
              Time accessedTime = 0, Time modifiedTime = 0, Time creationTime = 0) :
    Directory(name, accessedTime, modifiedTime, creationTime, inode, pFs, 0, pParent),
    m_Dir(dirRec), m_pFs(pFs)
  {}
  virtual ~Iso9660Directory() {};

  virtual void cacheDirectoryContents()
  {
    if(!m_pFs)
    {
        ERROR("ISO9660: m_pFs is null!");
        return;
    }
      
    Disk *myDisk = m_pFs->getDisk();

    // Grab our parent (will always be a directory)
    Iso9660Directory *pParentDir = reinterpret_cast<Iso9660Directory*>(m_pParent);
    if(pParentDir == 0)
    {
      // Root directory, . and .. should redirect to this directory
      Iso9660Directory *dot = new Iso9660Directory(String("."), m_Inode, m_pFs, m_pParent, m_Dir, m_AccessedTime, m_ModifiedTime, m_CreationTime);
      Iso9660Directory *dotdot = new Iso9660Directory(String(".."), m_Inode, m_pFs, m_pParent, m_Dir, m_AccessedTime, m_ModifiedTime, m_CreationTime);
      m_Cache.insert(String("."), dot);
      m_Cache.insert(String(".."), dotdot);
    }
    else
    {
      // Non-root, . and .. should point to the correct locations
      Iso9660Directory *dot = new Iso9660Directory(String("."), m_Inode, m_pFs, m_pParent, m_Dir, m_AccessedTime, m_ModifiedTime, m_CreationTime);
      m_Cache.insert(String("."), dot);

      Iso9660Directory *dotdot = new Iso9660Directory(String(".."),
                                                    pParentDir->getInode(),
                                                    pParentDir->m_pFs,
                                                    pParentDir->getParent(),
                                                    pParentDir->getDirRecord(),
                                                    pParentDir->getAccessedTime(),
                                                    pParentDir->getModifiedTime(),
                                                    pParentDir->getCreationTime()
                                                    );
      m_Cache.insert(String(".."), dotdot);
    }

    // How big is the directory?
    size_t dirSize = LITTLE_TO_HOST32(m_Dir.DataLen_LE);
    size_t dirLoc = LITTLE_TO_HOST32(m_Dir.ExtentLocation_LE);

    // Read the directory, block by block
    size_t numBlocks = (dirSize > 2048) ? dirSize / 2048 : 1;
    size_t i;
    for(i = 0; i < numBlocks; i++)
    {
      // Read the block
      uintptr_t block = myDisk->read((dirLoc + i) * 2048);

      // Complete, so start reading entries
      size_t offset = 0;
      bool bLastHit = false;
      while(offset < 2048)
      {
        Iso9660DirRecord *record = reinterpret_cast<Iso9660DirRecord*>(block + offset);
        offset += record->RecLen;

        if(record->RecLen == 0)
        {
          bLastHit = true;
          break;
        }
        else if(record->FileFlags & (1 << 0))
          continue;
        else if(record->FileFlags & (1 << 1) && record->FileIdentLen == 1)
        {
          if(record->FileIdent[0] == 0 || record->FileIdent[0] == 1)
            continue;
        }

        String fileName = m_pFs->parseName(*record);

        // Grab the UNIX timestamp
        Time unixTime = m_pFs->timeToUnix(record->Time);
        if(record->FileFlags & (1 << 1))
        {
          Iso9660Directory *dir = new Iso9660Directory(fileName, 0, m_pFs, this, *record, unixTime, unixTime, unixTime);
          m_Cache.insert(fileName, dir);
        }
        else
        {
          Iso9660File *file = new Iso9660File(fileName, unixTime, unixTime, unixTime, 0, m_pFs, LITTLE_TO_HOST32(record->DataLen_LE), *record, this);
          m_Cache.insert(fileName, file);
        }
      }

      // Last in the block, but are there still blocks to read?
      if(bLastHit && ((i + 1) == numBlocks))
        break;

      offset = 0;
    }

    m_bCachePopulated = true;
  }

  virtual bool addEntry(String filename, File *pFile, size_t type)
  {
    return false;
  }

  virtual bool removeEntry(File *pFile)
  {
    return false;
  }

  void fileAttributeChanged()
  {};

  inline Iso9660DirRecord &getDirRecord()
  {
    return m_Dir;
  }

private:
  // Our internal directory information (info about *this* directory, not the child)
  Iso9660DirRecord m_Dir;

  // Filesystem object
  Iso9660Filesystem *m_pFs;
};

#endif
