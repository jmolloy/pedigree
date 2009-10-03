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
#ifndef ISO9660FILESYSTEM_H
#define ISO9660FILESYSTEM_H

#include <vfs/Filesystem.h>
#include <utilities/List.h>
#include <utilities/Vector.h>
#include <utilities/Tree.h>
#include <process/Mutex.h>
#include <LockGuard.h>

#include "iso9660.h"

/** This class provides an implementation of the ISO9660 filesystem. */
class Iso9660Filesystem : public Filesystem
{
  friend class Iso9660File;
  friend class Iso9660Directory;

public:
  Iso9660Filesystem();

  virtual ~Iso9660Filesystem();

  //
  // Filesystem interface.
  //

  virtual bool initialise(Disk *pDisk);
  static Filesystem *probe(Disk *pDisk);
  virtual File* getRoot();
  virtual String getVolumeLabel();

protected:
  uintptr_t readBlock(File *pFile, uint64_t location);
  size_t getBlockSize()
  {return 2048;}

  virtual bool createFile(File* parent, String filename, uint32_t mask);
  virtual bool createDirectory(File* parent, String filename);
  virtual bool createSymlink(File* parent, String filename, String value);
  virtual bool remove(File* parent, File* file);

  virtual Disk *getDisk()
  {
    return m_pDisk;
  }

  String parseName(Iso9660DirRecord &name);
  String parseJolietName(Iso9660DirRecord &name);

  inline bool isLeap(uint32_t year)
  {
    if(year % 400 == 0) return true;
    else if(year % 100 == 0) return false;
    else if(year % 4 == 0) return true;
    else return false;
  }

  Time timeToUnix(Iso9660DirTimestamp &time)
  {
    Time ret = 0;

    ret += time.Second;
    ret += time.Minute * 60;
    ret += time.Hour * 60 * 60;
    ret += (time.Day - 1) * 24 * 60 * 60;

    static uint16_t cumulativeDays[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
    ret += cumulativeDays[time.Month - 1] * 24 * 60 * 60;

    uint32_t year = time.Year + 1900;
    if((time.Month) > 2 && isLeap(year))
      ret += 24 * 60 * 60;

    // Add leap days
    uint32_t realYear = year;
    uint32_t leapDays = ((realYear / 4) - (realYear / 100) + (realYear / 400));
    leapDays -= ((1970 / 4) - (1970 / 100) + (1970 / 400));

    ret += leapDays * 24 * 60 * 60;
    ret += (year - 1970) * 365 * 24 * 60 * 60;

    return ret;
  }

  File *fileFromDirRecord(Iso9660DirRecord &dir, size_t inodeNum, File *parent, bool bDirectory = false);

  Iso9660Filesystem(const Iso9660Filesystem&);
  void operator =(const Iso9660Filesystem&);

  /** Our raw device. */
  Disk *m_pDisk;

  /** Our superblock */
  Iso9660VolumeDescriptorPrimary m_PrimaryVolDesc;
  Iso9660VolumeDescriptorPrimary m_SuppVolDesc;
  Iso9660VolumeDescriptor m_VolDesc;
  Iso9660DirRecord *m_RootDir;

  /** Joliet level (0 if not using it) */
  int m_JolietLevel;

  /** Block size, and block number, of the disk */
  size_t m_BlockSize;
  size_t m_BlockNumber;

  /** Our volume label */
  String m_VolumeLabel;

  /** Root filesystem node. */
  File *m_pRoot;
};

#endif
