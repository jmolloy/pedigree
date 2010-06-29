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
#include <Module.h>
#include <utilities/utility.h>
#include <Log.h>
#include <utilities/List.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>
#include <syscallError.h>

#include <utilities/assert.h>
#include <utilities/PointerGuard.h>

#include "iso9660.h"
#include "Iso9660File.h"
#include "Iso9660Directory.h"

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

  // Read volume descriptors until the primary one is found
  // Sector 16 is the first sector on the disk
  bool bFound = false;
  for(size_t i = 16; i < 256; i++)
  {
    uintptr_t buff = m_pDisk->read(i * m_BlockSize);

    // Get the descriptor for this entry
    Iso9660VolumeDescriptor *vDesc = reinterpret_cast<Iso9660VolumeDescriptor*>(buff);
    if(strncmp(reinterpret_cast<const char*>(vDesc->Ident), "CD001", 5) != 0)
    {
      NOTICE("IDENT not correct for a descriptor, can't be an ISO9660 disk!");
      return false;
    }

    // Is this a primary descriptor?
    if(vDesc->Type == PRIM_VOL_DESC)
    {
      memcpy(&m_PrimaryVolDesc, reinterpret_cast<uint8_t*>(buff), sizeof(Iso9660VolumeDescriptorPrimary));
      bFound = true;
    }
    else if(vDesc->Type == SUPP_VOL_DESC)
    {
      memcpy(&m_SuppVolDesc, reinterpret_cast<uint8_t*>(buff), sizeof(Iso9660VolumeDescriptorPrimary));
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
    for(i = str.length() - 1; static_cast<int32_t>(i) >= 0; i--)
      if(str[i] != ' ')
        break;
    str = str.left(i + 1);
    m_VolumeLabel = String(str);
  }
  else
  {
    for(size_t i = 31; static_cast<int32_t>(i) >= 0; i--)
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

uintptr_t Iso9660Filesystem::readBlock(File *pFile, uint64_t location)
{
  // Sanity check.
  if (pFile->isDirectory())
    return 0;

  size_t size = 2048;

  Iso9660File *file = reinterpret_cast<Iso9660File*>(pFile);
  Iso9660DirRecord rec = file->getDirRecord();

  // Attempting to read past the EOF?
  if(location > pFile->getSize())
    return 0; // Impossible

  size_t blockSkip = location / m_BlockSize;
  size_t blockNum = LITTLE_TO_HOST32(rec.ExtentLocation_LE) + blockSkip;

  // Begin reading
  uintptr_t buff = m_pDisk->read(blockNum * m_BlockSize);

  return buff;
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
  Time t = timeToUnix(dir.Time);
  if(bDirectory)
  {
    Iso9660Directory *ret = new Iso9660Directory(fileName, inodeNum, this, parent, dir, t, t, t);
    return ret;
  }
  else
  {
    Iso9660File *ret = new Iso9660File(fileName, t, t, t, inodeNum, this, LITTLE_TO_HOST32(dir.DataLen_LE), dir, parent);
    return ret;
  }
}

static void initIso9660()
{
  VFS::instance().addProbeCallback(&Iso9660Filesystem::probe);
}

static void destroyIso9660()
{
}

MODULE_INFO("iso9660", &initIso9660, &destroyIso9660, "vfs");
