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

#include "VFS.h"
#include <Log.h>
#include <Module.h>
#include <utilities/utility.h>

VFS VFS::m_Instance;
File* VFS::m_EmptyFile = new File(String("invalid"), 0, 0, 0, 0, false, false, 0, 0, 0, false);

VFS &VFS::instance()
{
  return m_Instance;
}

File* VFS::invalidFile()
{
  return m_EmptyFile;
}

VFS::VFS() :
  m_Aliases(), m_ProbeCallbacks()
{
}

VFS::~VFS()
{
}

bool VFS::mount(Disk *pDisk, String &alias)
{
  for (List<Filesystem::ProbeCallback*>::Iterator it = m_ProbeCallbacks.begin();
       it != m_ProbeCallbacks.end();
       it++)
  {
    Filesystem::ProbeCallback cb = **it;
    Filesystem *pFs = cb(pDisk);
    if (pFs)
    {
      if (strlen(alias) == 0)
      {
        alias = pFs->getVolumeLabel();
      }
      alias = getUniqueAlias(alias);
      addAlias(pFs, alias);
      return true;
    }
  }
  return false;
}

void VFS::addAlias(Filesystem *pFs, String alias)
{
  pFs->m_nAliases++;
  Alias *pA = new Alias;
  pA->alias = alias;
  pA->fs = pFs;
  m_Aliases.pushBack(pA);
}

void VFS::addAlias(String oldAlias, String newAlias)
{
  for (List<Alias*>::Iterator it = m_Aliases.begin();
       it != m_Aliases.end();
       it++)
  {
    if (!strcmp(oldAlias, (*it)->alias))
    {
      Filesystem *pFs = (*it)->fs;
      pFs->m_nAliases++;
      Alias *pA = new Alias;
      pA->alias = newAlias;
      pA->fs = pFs;
      m_Aliases.pushBack(pA);
      return;
    }
  }
}

String VFS::getUniqueAlias(String alias)
{
  if(!aliasExists(alias))
    return alias;

  // <alias>-n is how we keep them unique
  // negative numbers already have a dash
  int32_t index = -1;
  while(true)
  {
    NormalStaticString tmpAlias;
    tmpAlias += static_cast<const char*>(alias);
    tmpAlias.append(index);

    String s = String(static_cast<const char*>(tmpAlias));
    if(!aliasExists(s))
      return s;
    index--;
  }

  return String();
}


bool VFS::aliasExists(String alias)
{
  for (List<Alias*>::Iterator it = m_Aliases.begin();
       it != m_Aliases.end();
       it++)
  {
    if (!strcmp(static_cast<const char*>(alias), static_cast<const char *>((*it)->alias)))
    {
      return true; // alias exists!
    }
  }
  return false; // doesn't exist
}

void VFS::removeAlias(String alias)
{
  for (List<Alias*>::Iterator it = m_Aliases.begin();
       it != m_Aliases.end();
       it++)
  {
    if (alias == (*it)->alias)
    {
      Filesystem *pFs = (*it)->fs;
      Alias *pA = *it;
      m_Aliases.erase(it);
      delete pA;

      pFs->m_nAliases--;
      if (pFs->m_nAliases == 0)
      {
        delete pFs;
      }
    }
  }
}

void VFS::removeAllAliases(Filesystem *pFs)
{
  for (List<Alias*>::Iterator it = m_Aliases.begin();
       it != m_Aliases.end();
       it++)
  {
    if (pFs == (*it)->fs)
    {
      Alias *pA = *it;
      m_Aliases.erase(it);
      delete pA;
    }
  }
  delete pFs;
}

Filesystem *VFS::lookupFilesystem(String alias)
{
  for (List<Alias*>::Iterator it = m_Aliases.begin();
       it != m_Aliases.end();
       it++)
  {
    if (!strcmp(alias, (*it)->alias))
    {
      return (*it)->fs;
    }
  }
  return 0;
}

File *VFS::find(String path, File *pStartNode)
{
  // Search for a colon.
  bool bColon = false;
  size_t i;
  for (i = 0; i < path.length(); i++)
  {
    if (path[i] == ':')
    {
      bColon = true;
      break;
    }
  }

  if (!bColon)
  {
    // Pass directly through to the filesystem, if one specified.
    if (!pStartNode) return VFS::invalidFile();
    else return pStartNode->getFilesystem()->find(path, pStartNode);
  }
  else
  {
    String newPath = path.split(i+1);
    path.chomp();
    
    
    // Attempt to find a filesystem alias.
    Filesystem *pFs = lookupFilesystem(path);
    if (!pFs)
      return VFS::invalidFile();
    return pFs->find(newPath, pStartNode);
  }
}

void VFS::addProbeCallback(Filesystem::ProbeCallback callback)
{
  Filesystem::ProbeCallback *p = new Filesystem::ProbeCallback;
  *p = callback;
  m_ProbeCallbacks.pushBack(p);
}

bool VFS::createFile(String path, uint32_t mask, File *pStartNode)
{
  // Search for a colon.
  bool bColon = false;
  size_t i;
  for (i = 0; i < path.length(); i++)
  {
    if (path[i] == ':')
    {
      bColon = true;
      break;
    }
  }

  if (!bColon)
  {
    // Pass directly through to the filesystem, if one specified.
    if (!pStartNode) return false;
    else return pStartNode->getFilesystem()->createFile(path, mask, pStartNode);
  }
  else
  {
    String newPath = path.split(i+1);
    path.chomp();
    
    
    // Attempt to find a filesystem alias.
    Filesystem *pFs = lookupFilesystem(path);
    if (!pFs)
      return false;
    return pFs->createFile(newPath, mask, pStartNode);
  }
}

bool VFS::createDirectory(String path, File *pStartNode)
{
  // Search for a colon.
  bool bColon = false;
  size_t i;
  for (i = 0; i < path.length(); i++)
  {
    if (path[i] == ':')
    {
      bColon = true;
      break;
    }
  }

  if (!bColon)
  {
    // Pass directly through to the filesystem, if one specified.
    if (!pStartNode) return false;
    else return pStartNode->getFilesystem()->createDirectory(path, pStartNode);
  }
  else
  {
    String newPath = path.split(i+1);
    path.chomp();
    
    
    // Attempt to find a filesystem alias.
    Filesystem *pFs = lookupFilesystem(path);
    if (!pFs)
      return false;
    return pFs->createDirectory(newPath, pStartNode);
  }
}

bool VFS::createSymlink(String path, String value, File *pStartNode)
{
  // Search for a colon.
  bool bColon = false;
  size_t i;
  for (i = 0; i < path.length(); i++)
  {
    if (path[i] == ':')
    {
      bColon = true;
      break;
    }
  }

  if (!bColon)
  {
    // Pass directly through to the filesystem, if one specified.
    if (!pStartNode) return false;
    else return pStartNode->getFilesystem()->createSymlink(path, value, pStartNode);
  }
  else
  {
    String newPath = path.split(i+1);
    path.chomp();
    
    
    // Attempt to find a filesystem alias.
    Filesystem *pFs = lookupFilesystem(path);
    if (!pFs)
      return false;
    return pFs->createSymlink(newPath, value, pStartNode);
  }
}

bool VFS::remove(String path, File *pStartNode)
{
  // Search for a colon.
  bool bColon = false;
  size_t i;
  for (i = 0; i < path.length(); i++)
  {
    if (path[i] == ':')
    {
      bColon = true;
      break;
    }
  }

  if (!bColon)
  {
    // Pass directly through to the filesystem, if one specified.
    if (!pStartNode) return false;
    else return pStartNode->getFilesystem()->remove(path, pStartNode);
  }
  else
  {
    String newPath = path.split(i+1);
    path.chomp();
    
    
    // Attempt to find a filesystem alias.
    Filesystem *pFs = lookupFilesystem(path);
    if (!pFs)
      return false;
    return pFs->remove(newPath, pStartNode);
  }
}

void initVFS()
{
}

void destroyVFS()
{
}

MODULE_NAME("VFS");
MODULE_ENTRY(&initVFS);
MODULE_EXIT(&destroyVFS);
MODULE_DEPENDS(0);

