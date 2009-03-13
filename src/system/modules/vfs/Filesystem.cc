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

#include "Filesystem.h"
#include <Log.h>
#include <utilities/utility.h>
#include <processor/Processor.h>

Filesystem::Filesystem() :
  m_nAliases(0), m_bReadOnly(false)
{
}

/// \todo This algorithm needs reworking - it can be so much better!
File Filesystem::find(String path)
{
  // We expect a leading '/'.
  char *cPath = const_cast<char*> (static_cast<const char*> (path));
  char *pathSegment;

  File curDir = getRoot();

  if (cPath[0] != '/')
    ERROR("Filesystem::find - path component malformed! (" << cPath << ")");

  cPath++;
  while (cPath[0])
  {
    bool terminate = false;

    pathSegment = cPath;

    while (cPath[0] != '\0' && cPath[0] != '/')
      cPath++;

    if (cPath[0] == '\0') terminate = true;

    cPath[0] = '\0';

    if (pathSegment == cPath) // Multiple '/''s - just ignore and continue.
    {
      cPath++;
      continue;
    }

    if (!curDir.isDirectory())
    {
      // Error - is directory!
      return File();
    }

    bool bFound = false;
    for (File f = curDir.firstChild();
         f.isValid();
         f = curDir.nextChild())
    {
      String s = f.getName();
      if (!strcmp(pathSegment, static_cast<const char*> (s)))
      {
        bFound = true;
        curDir = f;
        break;
      }
    }
    if (!bFound)
    {
      // Error - not found!
      return File();
    }

    cPath++;
    if (cPath[0] == '\0' || terminate)
    {
      return curDir;
    }

  }
  return curDir;
}

bool Filesystem::createFile(String path, uint32_t mask)
{
  // We expect a leading '/'.
  char *cPath = const_cast<char*> (static_cast<const char*> (path));
  char *pathSegment;

  File curDir = getRoot();

  if (cPath[0] != '/')
    ERROR("Filesystem::find - path component malformed! (" << cPath << ")");

  cPath++;
  while (cPath[0])
  {
    bool terminate = false;

    pathSegment = cPath;

    while (cPath[0] != '\0' && cPath[0] != '/')
      cPath++;

    if (cPath[0] == '\0') terminate = true;

    cPath[0] = '\0';

    if (!curDir.isDirectory())
    {
      // Error - is directory!
      return false;
    }

    bool bFound = false;
    for (File f = curDir.firstChild();
         f.isValid();
         f = curDir.nextChild())
    {
      if (!strcmp(pathSegment, static_cast<const char*> (f.getName())))
      {
        bFound = true;
        curDir = f;
        break;
      }
    }
    if (!bFound && terminate) // TODO cover case with trailing '/'
    {
      // Not found, but nothing left, so create here.
      createFile(curDir, String(pathSegment), mask);
      return true;
    }
    else if(!bFound && terminate)
    {
      // Error - not found!
      return false;
    }

    cPath++;
    if (cPath[0] == '\0' || terminate)
    {
      return false;
    }

  }
  return false;
}

bool Filesystem::createDirectory(String path)
{
  // We expect a leading '/'.
  char *cPath = const_cast<char*> (static_cast<const char*> (path));
  char *pathSegment;

  File curDir = getRoot();

  if (cPath[0] != '/')
    ERROR("Filesystem::find - path component malformed! (" << cPath << ")");

  cPath++;
  while (cPath[0])
  {
    bool terminate = false;

    pathSegment = cPath;

    while (cPath[0] != '\0' && cPath[0] != '/')
      cPath++;

    if (cPath[0] == '\0') terminate = true;

    cPath[0] = '\0';

    if (!curDir.isDirectory())
    {
      // Error - is directory!
      return false;
    }

    bool bFound = false;
    for (File f = curDir.firstChild();
         f.isValid();
         f = curDir.nextChild())
    {
      if (!strcmp(pathSegment, static_cast<const char*> (f.getName())))
      {
        bFound = true;
        curDir = f;
        break;
      }
    }
    if (!bFound && terminate) // TODO cover case with trailing '/'
    {
      // Not found, but nothing left, so create here.
      createDirectory(curDir, String(pathSegment));
      return true;
    }
    else
    {
      // Error - not found!
      return false;
    }

    cPath++;
    if (cPath[0] == '\0' || terminate)
    {
      return false;
    }

  }
  return false;
}

bool Filesystem::createSymlink(String path, String value)
{
  // We expect a leading '/'.
  char *cPath = const_cast<char*> (static_cast<const char*> (path));
  char *pathSegment;

  File curDir = getRoot();

  if (cPath[0] != '/')
    ERROR("Filesystem::find - path component malformed! (" << cPath << ")");

  cPath++;
  while (cPath[0])
  {
    bool terminate = false;

    pathSegment = cPath;

    while (cPath[0] != '\0' && cPath[0] != '/')
      cPath++;

    if (cPath[0] == '\0') terminate = true;

    cPath[0] = '\0';

    if (!curDir.isDirectory())
    {
      // Error - is directory!
      return false;
    }

    bool bFound = false;
    for (File f = curDir.firstChild();
         f.isValid();
         f = curDir.nextChild())
    {
      if (!strcmp(pathSegment, static_cast<const char*> (f.getName())))
      {
        bFound = true;
        curDir = f;
        break;
      }
    }
    if (!bFound && terminate) // TODO cover case with trailing '/'
    {
      // Not found, but nothing left, so create here.
      createSymlink(curDir, String(pathSegment), value);
      return true;
    }
    else
    {
      // Error - not found!
      return false;
    }

    cPath++;
    if (cPath[0] == '\0' || terminate)
    {
      return false;
    }

  }
  return false;
}

bool Filesystem::remove(String path)
{
  // We expect a leading '/'.
  char *cPath = const_cast<char*> (static_cast<const char*> (path));
  char *pathSegment;

  File parent = File();
  File curDir = getRoot();

  if (cPath[0] != '/')
    ERROR("Filesystem::find - path component malformed! (" << cPath << ")");

  cPath++;
  while (cPath[0])
  {
    bool terminate = false;

    pathSegment = cPath;

    while (cPath[0] != '\0' && cPath[0] != '/')
      cPath++;

    if (cPath[0] == '\0') terminate = true;

    cPath[0] = '\0';

    if (!curDir.isDirectory())
    {
      // Error - is directory!
      return false;
    }

    bool bFound = false;
    for (File f = curDir.firstChild();
         f.isValid();
         f = curDir.nextChild())
    {
      if (!strcmp(pathSegment, static_cast<const char*> (f.getName())))
      {
        bFound = true;
        parent = curDir;
        curDir = f;
        break;
      }
    }

    if (!bFound)
    {
      // Error - not found!
      return false;
    }

    cPath++;
    if (cPath[0] == '\0' || terminate)
    {
      return remove(parent, curDir);
    }

  }
  return remove(parent, curDir);
}
