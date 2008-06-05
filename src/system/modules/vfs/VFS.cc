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
#include <Module.h>

static VFS VFS::m_Instance;

VFS &VFS::instance()
{
  return m_Instance;
}

VFS::VFS()
{
}

VFS::~VFS()
{
}

void VFS::addAlias(Filesystem *pFs, String alias)
{
  pFs->nAliases++;
  Alias *pA = new Alias;
  pA->alias = alias;
  pA->fs = pFs;
  m_Aliases.pushBack(pA);
}

void VFS::removeAlias(String alias)
{
  for (List<Alias*>::Iterator it = m_Aliases.begin();
       it != m_Aliases.end();
       it++)
  {
    if (alias == (*it)->alias))
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
    if (alias == (*it)->alias)
    {
      return (*it)->pFs;
    }
  }
  return 0;
}

void initVFS()
{
}

void destroyVFS()
{
}

const char *g_pModuleName = "VFS";
ModuleEntry g_pModuleEntry = &initVFS;
ModuleExit  g_pModuleExit  = &destroyVFS;
