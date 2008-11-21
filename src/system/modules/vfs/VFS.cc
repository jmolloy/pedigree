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

VFS &VFS::instance()
{
  return m_Instance;
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
      addAlias(pFs, alias);
      return true;
    }
  }
  return false;
}

void VFS::addAlias(Filesystem *pFs, String alias)
{
  pFs->nAliases++;
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
      pFs->nAliases++;
      Alias *pA = new Alias;
      pA->alias = newAlias;
      pA->fs = pFs;
      m_Aliases.pushBack(pA);
      return;
    }
  }
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

      pFs->nAliases--;
      if (pFs->nAliases == 0)
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

File VFS::find(String path)
{
  // We expect a colon. If we don't find it, we cry loudly.
  char *cPath = const_cast<char*> (static_cast<const char*> (path));
  char *newPath = cPath;
  while (cPath[0] != '\0' && cPath[0] != ':')
    cPath++;
  if (cPath[0] == '\0')
  {
    // Error - malformed path!
    return File();
  }

  cPath[0] = '\0';
  cPath++;

  // Attempt to find a filesystem alias.
  Filesystem *pFs = lookupFilesystem(String(newPath));
  if (!pFs)
  {
    // Error - filesystem not found!
    return File();
  }
  File a = pFs->find(String(cPath));
  
  return  a;
  
}

void VFS::addProbeCallback(Filesystem::ProbeCallback callback)
{
  Filesystem::ProbeCallback *p = new Filesystem::ProbeCallback;
  *p = callback;
  m_ProbeCallbacks.pushBack(p);
}

bool VFS::createFile(String path)
{
  // We expect a colon. If we don't find it, we cry loudly.
  char *cPath = const_cast<char*> (static_cast<const char*> (path));
  char *newPath = cPath;
  while (cPath[0] != '\0' && cPath[0] != ':')
    cPath++;
  if (cPath[0] == '\0')
  {
    // Error - malformed path!
    return false;
  }

  cPath[0] = '\0';
  cPath++;

  // Attempt to find a filesystem alias.
  Filesystem *pFs = lookupFilesystem(String(newPath));
  if (!pFs)
  {
    // Error - filesystem not found!
    return false;
  }
  return pFs->createFile(String(cPath));
}
  
bool VFS::createDirectory(String path)
{
  // We expect a colon. If we don't find it, we cry loudly.
  char *cPath = const_cast<char*> (static_cast<const char*> (path));
  char *newPath = cPath;
  while (cPath[0] != '\0' && cPath[0] != ':')
    cPath++;
  if (cPath[0] == '\0')
  {
    // Error - malformed path!
    return false;
  }

  cPath[0] = '\0';
  cPath++;

  // Attempt to find a filesystem alias.
  Filesystem *pFs = lookupFilesystem(String(newPath));
  if (!pFs)
  {
    // Error - filesystem not found!
    return false;
  }
  return pFs->createDirectory(String(cPath));
}

bool VFS::createSymlink(String path, String value)
{
  // We expect a colon. If we don't find it, we cry loudly.
  char *cPath = const_cast<char*> (static_cast<const char*> (path));
  char *newPath = cPath;
  while (cPath[0] != '\0' && cPath[0] != ':')
    cPath++;
  if (cPath[0] == '\0')
  {
    // Error - malformed path!
    return false;
  }

  cPath[0] = '\0';
  cPath++;

  // Attempt to find a filesystem alias.
  Filesystem *pFs = lookupFilesystem(String(newPath));
  if (!pFs)
  {
    // Error - filesystem not found!
    return false;
  }
  return pFs->createSymlink(String(cPath), value);
}

bool VFS::remove(String path)
{
  // We expect a colon. If we don't find it, we cry loudly.
  char *cPath = const_cast<char*> (static_cast<const char*> (path));
  char *newPath = cPath;
  while (cPath[0] != '\0' && cPath[0] != ':')
    cPath++;
  if (cPath[0] == '\0')
  {
    // Error - malformed path!
    return false;
  }

  cPath[0] = '\0';
  cPath++;

  // Attempt to find a filesystem alias.
  Filesystem *pFs = lookupFilesystem(String(newPath));
  if (!pFs)
  {
    // Error - filesystem not found!
    return false;
  }
  return pFs->remove(String(cPath));
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

