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

#include "Iso9660Filesystem.h"
#include <vfs/VFS.h>
#include <vfs/Directory.h>
#include <Module.h>
#include <utilities/utility.h>
#include <Log.h>
#include <utilities/List.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>
#include <syscallError.h>

#include <utilities/PointerGuard.h>

#include "iso9660.h"

String WideToMultiByteStr(uint8_t *in, size_t inLen, size_t maxLen)
{
  NormalStaticString ret;
  ret.clear();

  while((*in || in[1]) && (inLen > 0) && (maxLen > 0))
  {
    uint16_t c = (*in << 8) | in[1];
    if(c > 0x7f)
    {
      /// \todo Handle UTF
    }
    else
      ret.append(static_cast<char>(c));

    in += 2;
    inLen--;
  }
  ret.append('\0');

  return String(ret);
}

class Iso9660File : public File
{
  friend class Iso9660Directory;

private:
  /** Copy constructors are hidden - unused! */
  Iso9660File(const Iso9660File &);
  Iso9660File& operator =(const File&);
  Iso9660File& operator =(const Iso9660File&);
public:
  /** Constructor, should be called only by a Filesystem. */
  Iso9660File(String name, Time accessedTime, Time modifiedTime, Time creationTime,
       uintptr_t inode, class Iso9660Filesystem *pFs, size_t size, Iso9660DirRecord &record, File *pParent = 0) :
    File(name,accessedTime,modifiedTime,creationTime,inode,pFs,size,pParent), m_Dir(record), m_pFs(pFs)
  {}
  virtual ~Iso9660File() {}

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
    Directory(name, accessedTime, modifiedTime, creationTime, inode, pFs, 0, pParent), m_Dir(dirRec), m_pFs(pFs)
  {}
  virtual ~Iso9660Directory() {};

  uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer)
  {return 0;}
  uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer)
  {return 0;}

  void truncate()
  {}

  virtual void cacheDirectoryContents()
  {
    Disk *myDisk = m_pFs->getDisk();

    // Grab our parent (will always be a directory)
    Iso9660Directory *pParentDir = reinterpret_cast<Iso9660Directory*>(m_pParent->getParent());
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
      uint8_t *block = new uint8_t[2048];
      PointerGuard<uint8_t> blockGuard(block);
      memset(block, 0, 2048);

      if(myDisk->read((dirLoc + i) * 2048, 2048, reinterpret_cast<uintptr_t>(block)) == 0)
      {
        NOTICE("Couldn't read disk!");
        return;
      }

      // Complete, so start reading entries
      size_t offset = 0;
      bool bLastHit = false;
      while(offset < 2048)
      {
        Iso9660DirRecord *record = reinterpret_cast<Iso9660DirRecord*>(reinterpret_cast<uintptr_t>(block) + offset);
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

Iso9660Filesystem::Iso9660Filesystem() :
  m_pDisk(0), m_PrimaryVolDesc(), m_SuppVolDesc(), m_VolDesc(), m_RootDir(0),
  m_JolietLevel(0), m_BlockSize(0), m_BlockNumber(0), m_VolumeLabel(""), m_pRoot(0)
{
}

Iso9660Filesystem::~Iso9660Filesystem()
{
}

bool Iso9660Filesystem::initialise(Disk *pDisk)
{
  m_pDisk = pDisk;

  // Only work on ATAPI disks
  if(m_pDisk->getSubType() != Disk::ATAPI)
  {
    WARNING("Not trying to find an ISO9660 filesystem on a non-ATAPI device");
    return false;
  }

  /// \todo Obtain disk information (perhaps a new call in Disk?)
  m_BlockSize = 2048;
  m_BlockNumber = 0;

  uint8_t *tmpBuff = new uint8_t[m_BlockSize];
  PointerGuard<uint8_t> tmpBuffGuard(tmpBuff);

  // Read volume descriptors until the primary one is found
  // Sector 16 is the first sector on the disk
  size_t tmpInt = 0;
  bool bFound = false;
  for(size_t i = 16; i < 256; i++)
  {
    tmpInt = m_pDisk->read(i * m_BlockSize, m_BlockSize, reinterpret_cast<uintptr_t>(tmpBuff));
    if(!tmpInt || !(tmpInt == m_BlockSize))
      return false;

    // Get the descriptor for this entry
    Iso9660VolumeDescriptor *vDesc = reinterpret_cast<Iso9660VolumeDescriptor*>(&tmpBuff[0]);
    if(strncmp(reinterpret_cast<const char*>(vDesc->Ident), "CD001", 5) != 0)
    {
      NOTICE("IDENT not correct for a descriptor, can't be an ISO9660 disk!");
      return false;
    }

    // Is this a primary descriptor?
    if(vDesc->Type == PRIM_VOL_DESC)
    {
      memcpy(&m_PrimaryVolDesc, tmpBuff, sizeof(Iso9660VolumeDescriptorPrimary));
      bFound = true;
    }
    else if(vDesc->Type == SUPP_VOL_DESC)
    {
      memcpy(&m_SuppVolDesc, tmpBuff, sizeof(Iso9660VolumeDescriptorPrimary));
      bFound = true;

      // Figure out the Joliet level
      if(m_SuppVolDesc.Unused3_EscSequences[0] == 0x25 && m_SuppVolDesc.Unused3_EscSequences[1] == 0x2F)
      {
        if(m_SuppVolDesc.Unused3_EscSequences[2] == 0x40)
          m_JolietLevel = 1;
        else if(m_SuppVolDesc.Unused3_EscSequences[2] == 0x43)
          m_JolietLevel = 2;
        else if(m_SuppVolDesc.Unused3_EscSequences[2] == 0x45)
          m_JolietLevel = 3;
        else
          NOTICE("No Joliet level found");
      }
      else
        NOTICE("Not handling Joliet level");
    }
    else if(vDesc->Type == TERM_VOL_DESC)
      break;
  }

  if(!bFound)
  {
    WARNING("ISO9660: Neither a primary or supplementary volume descriptor was found");
    return false;
  }

  if(m_JolietLevel)
  {
    // In this case, the supplementary descriptor is actually the main one
    m_PrimaryVolDesc = m_SuppVolDesc;
  }

  // Grab the volume label, properly trimmed
  char *volLabel = new char[32];
  memcpy(volLabel, m_PrimaryVolDesc.VolIdent, 32);
  if(m_JolietLevel)
  {
    String volLabelString = WideToMultiByteStr(reinterpret_cast<uint8_t*>(volLabel), 32, 32);
    NormalStaticString str;
    str.append(volLabelString);

    size_t i;
    for(i = str.length() - 1; i >= 0; i--)
      if(str[i] != ' ')
        break;
    str = str.left(i + 1);
    m_VolumeLabel = String(str);
  }
  else
  {
    for(size_t i = 31; i >= 0; i--)
    {
      if(volLabel[i] != ' ')
      {
        volLabel[i + 1] = 0;
        break;
      }
    }
    m_VolumeLabel = String(volLabel);
  }

  delete [] volLabel;

  m_RootDir = reinterpret_cast<Iso9660DirRecord*>(m_PrimaryVolDesc.RootDirRecord);
  m_pRoot = fileFromDirRecord(*m_RootDir, 1, 0, true);

  return true;
}

Filesystem *Iso9660Filesystem::probe(Disk *pDisk)
{
  Iso9660Filesystem *pFs = new Iso9660Filesystem();
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

File* Iso9660Filesystem::getRoot()
{
  return m_pRoot;
}

String Iso9660Filesystem::getVolumeLabel()
{
  // Is there a volume label already?
  if(m_PrimaryVolDesc.VolIdent[0] != ' ')
    return m_VolumeLabel;

  // none found, do a default
  NormalStaticString str;
  str += "no-volume-label@";
  str.append(reinterpret_cast<uintptr_t>(this), 16);
  return String(static_cast<const char*>(str));
}

uint64_t Iso9660Filesystem::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  // Sanity check.
  if (pFile->isDirectory())
    return 0;

  Iso9660File *file = reinterpret_cast<Iso9660File*>(pFile);
  Iso9660DirRecord rec = file->getDirRecord();

  // Ensure we're not going to write beyond the end of the file
  if((location + size) > pFile->getSize())
    size = pFile->getSize() - location;

  uint8_t *dest = reinterpret_cast<uint8_t*>(buffer);
  size_t offset = location % m_BlockSize;
  size_t bytesWritten = 0;
  size_t bytesToGo = size;
  size_t blockSkip = location / m_BlockSize;
  size_t blockNum = LITTLE_TO_HOST32(rec.ExtentLocation_LE) + blockSkip;

  // Begin reading
  while(bytesToGo)
  {
    uint8_t *tmp = new uint8_t[m_BlockSize];
    PointerGuard<uint8_t> tmpGuard(tmp);

    m_pDisk->read(blockNum * m_BlockSize, m_BlockSize, reinterpret_cast<uintptr_t>(tmp));

    size_t numToCopy = 0;
    if(bytesToGo < (m_BlockSize - offset))
      numToCopy = bytesToGo;
    else
      numToCopy = (m_BlockSize - offset);

    memcpy((dest + bytesWritten), (tmp + offset), numToCopy);
    bytesWritten += numToCopy;
    bytesToGo -= numToCopy;

    offset = 0;
    blockNum++;
  }
  if(dest[bytesWritten] == 0xca || dest[bytesWritten - 1] == 0xca || dest[bytesWritten - 2] == 0xca)
    NOTICE("a: " << dest[bytesWritten] << ", b: " << dest[bytesWritten-1] << ", c: " << dest[bytesWritten-2] << ".");
  dest[bytesWritten] = 0;

  return bytesWritten;
}

uint64_t Iso9660Filesystem::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  WARNING("Can't write to an ISO9660 filesystem");
  return 0;
}

void Iso9660Filesystem::fileAttributeChanged(File *pFile)
{
}

char toUpper(char c)
{
  if(c < 'a' || c > 'z')
    return c; // special chars
  c += ('A' - 'a');
  return c;
}

char toLower(char c)
{
  if(c < 'A' || c > 'Z')
    return c; // special chars
  c -= ('A' - 'a');
  return c;
}

void Iso9660Filesystem::truncate(File *pFile)
{
}

bool Iso9660Filesystem::createFile(File *parent, String filename, uint32_t mask)
{
  return false;
}

bool Iso9660Filesystem::createDirectory(File* parent, String filename)
{
  return false;
}

bool Iso9660Filesystem::createSymlink(File* parent, String filename, String value)
{
  return false;
}

bool Iso9660Filesystem::remove(File* parent, File* file)
{
  return false;
}

String Iso9660Filesystem::parseName(Iso9660DirRecord &dirRecord)
{
  if(m_JolietLevel)
    return parseJolietName(dirRecord);

  NormalStaticString ret;
  ret.clear();

  // ISO9660 maximum name length is 31 bytes
  size_t len = (dirRecord.FileIdentLen < 31) ? dirRecord.FileIdentLen : 31;
  size_t i;
  for(i = 0; i < len; i++)
  {
    if(dirRecord.FileIdent[i] == ';')
      break;
    else
      ret.append(toLower(static_cast<char>(dirRecord.FileIdent[i])));
  }

  if(i && (dirRecord.FileIdent[i - 1] == '.'))
    ret.stripLast();
  ret.append('\0');

  return String(ret);
}

String Iso9660Filesystem::parseJolietName(Iso9660DirRecord &name)
{
  String s = WideToMultiByteStr(name.FileIdent, name.FileIdentLen >> 1, 64);
  NormalStaticString str;
  str.append(s);
  size_t len = str.length();

  if((len > 2) && (str[len - 2] == ';') && (str[len - 1] == '1'))
    len -= 2;

  while(len >= 2 && (str[len - 1] == '.'))
    len--;
  str = str.left(len);

  return String(str);
}

File *Iso9660Filesystem::fileFromDirRecord(Iso9660DirRecord &dir, size_t inodeNum, File *parent, bool bDirectory)
{
  String fileName = parseName(dir);
  if(bDirectory)
  {
    Iso9660Directory *ret = new Iso9660Directory(fileName, inodeNum, this, parent, dir);
    return ret;
  }
  else
  {
    File *ret = new File(fileName, 0, 0, 0, inodeNum, this, HOST_TO_LITTLE32(dir.DataLen_LE), parent);
    return ret;
  }
}

void initIso9660()
{
  VFS::instance().addProbeCallback(&Iso9660Filesystem::probe);
}

void destroyIso9660()
{
}

MODULE_NAME("iso9660");
MODULE_ENTRY(&initIso9660);
MODULE_EXIT(&destroyIso9660);
MODULE_DEPENDS("VFS");
