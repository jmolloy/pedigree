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
#include "Filesystem.h"
#include <Log.h>
#include <utilities/utility.h>
#include <processor/Processor.h>

Filesystem::Filesystem() :
  m_nAliases(0), m_bReadOnly(false)
{
}

File* Filesystem::find(String path)
{
  File* parent = VFS::invalidFile();

  File* ret = findNode(path, parent);
  
  return ret;
}

bool Filesystem::createFile(String path, uint32_t mask)
{
  File* parent = VFS::invalidFile();
  File* file = findNode(path, parent);

  if (file->isValid())
  {
    // File existed.
    return false;
  }
  else
  {
    // To calculate the filename, grab the last token in the tokenised path.
    /// \todo Logic faulty, should actually use the canonical path.
    List<String*> tokens = path.tokenise('/');
    String filename;
    for (List<String*>::Iterator it = tokens.begin();
         it != tokens.end();
         it++)
    {
      String *pStr = *it;
      if (pStr->length() > 0)
        filename = *pStr;
      delete pStr;
    }
    createFile(parent, filename, mask);
    
    return true;
  }
}

bool Filesystem::createDirectory(String path)
{
  File* parent;
  File* file = findNode(path, parent);

  if (file->isValid())
  {
    // File existed.
    delete parent;
    return false;
  }
  else
  {
    // To calculate the filename, grab the last token in the tokenised path.
    /// \todo Logic faulty, should actually use the canonical path.
    List<String*> tokens = path.tokenise('/');
    String filename;
    for (List<String*>::Iterator it = tokens.begin();
         it != tokens.end();
         it++)
    {
      String *pStr = *it;
      if (pStr->length() > 0)
        filename = *pStr;
      delete pStr;
    }
    createDirectory(parent, filename);
    
    delete parent;

    return true;
  }
}

bool Filesystem::createSymlink(String path, String value)
{
  File* parent;
  File* file = findNode(path, parent);

  if (file->isValid())
  {
    // File existed.
    delete parent;
    return false;
  }
  else
  {
    // To calculate the filename, grab the last token in the tokenised path.
    /// \todo Logic faulty, should actually use the canonical path.
    List<String*> tokens = path.tokenise('/');
    String filename;
    for (List<String*>::Iterator it = tokens.begin();
         it != tokens.end();
         it++)
    {
      String *pStr = *it;
      if (pStr->length() > 0)
        filename = *pStr;
      delete pStr;
    }
    createSymlink(parent, filename, value);
    
    delete parent;

    return true;
  }
}

bool Filesystem::remove(String path)
{
  File* parent;

  File* file = findNode(path, parent);

  /// \todo Needs removing from cache.
  bool ret = remove(parent, file);
  
  delete parent;
  
  delete file;
  
  return ret;
}

File* Filesystem::findNode(String path, File*& parent)
{

  List<String*> tokens;
  canonisePath(path, tokens);

  // Attempt to fetch from cache.
  File* curDir = cacheLookup(tokens, parent);
  if (!curDir->isValid())
  {
    curDir = getRoot();
    String curStr;
    for (List<String*>::Iterator it = tokens.begin();
        it != tokens.end();
        it++)
    {
      String *pStr = *it;

      if (pStr->length() == 0)
        continue;

      if (!curDir->isDirectory())
      {
        // Error - not a directory!
        return VFS::invalidFile();
      }
      /// \todo Check for symlinks and follow.

      parent = curDir;
      bool bFound = false;
      for (File* f = curDir->firstChild();
          f->isValid();
          f = curDir->nextChild())
      {
        String s = f->getName();

        String tmp = curStr;
        tmp += "/";
        tmp += s;

        // Add to cache.
        // cacheInsert(tmp, f);

        if (*pStr == s)
        {
          bFound = true;
          curDir = f;
          curStr = tmp;
          break;
        }
      }
      if (!bFound)
        return VFS::invalidFile();
    }
  }

  for (List<String*>::Iterator it = tokens.begin();
       it != tokens.end();
       it++)
  {
    delete *it;
  }

  return curDir;
}

void Filesystem::canonisePath(String &path, List<String*> &tokens)
{
  // Tokenise the string to remove any '..' or '.' references.
  tokens = path.tokenise('/');

  bool bStop = false;
  while (!bStop)
  {
    bStop = true;
    List<String*>::Iterator iParent = tokens.end();
    for (List<String*>::Iterator it = tokens.begin();
        it != tokens.end();
        it++)
    {
      String *pStr = *it;
      if (!strcmp(*pStr, "."))
      {
        // '.' - just delete this path segment.
        delete pStr;
        tokens.erase(it);
        bStop = false;
        break;
      }
      else if (!strcmp(*pStr, ".."))
      {
        // '..' - delete this path segment and the preceding one (iParent). This is done by erasing iParent and then erasing
        // the returned Iterator (points to the next valid item).
        delete pStr;
        if (iParent != tokens.end())
        {
          delete *iParent;
          it = tokens.erase(iParent);
        }
        tokens.erase(it);
        bStop = false;
        break;
      }
      else if (pStr->length() == 0)
      {
        // Pointless node, remove.
        delete pStr;
        tokens.erase(it);
        bStop = false;
        break;
      }
      iParent = it;
    }
  }
}

File* Filesystem::cacheLookup(List<String*> &canonicalPath, File* & parent)
{
  // What are we searching for? the file given by canonicalPath or its parent directory?
  bool bLookingForParent = false;

  File* theFile;

  // If there are no path segments (i.e. we're looking for the root), just return root.
  if (canonicalPath.count() == 0)
  {
    parent = VFS::invalidFile();
    return getRoot();
  }

  // This algorithm will not return partial matches.
  int i = canonicalPath.count();
  while (true)
  {
    // Create a String from path segments 0..i exclusive.
    String constructedPath;
    size_t j = 0;
    for (List<String*>::Iterator it = canonicalPath.begin();
         j < i;
         it++)
    {
      constructedPath += "/";
      constructedPath += **it;
      j++;
    }

    // Look for the file.
    File *file = m_Cache.lookup(constructedPath);
    if (bLookingForParent)
    {
      // If we were looking for the parent, we can assign the parent and return.
      parent = (file) ? file : VFS::invalidFile();
      return theFile;
    }
    else
    {
      // Else we were looking for the normal file - assign it and start looking for the parent.
      theFile = (file) ? file : VFS::invalidFile();
      bLookingForParent = true;
      // To search for the parent, look one path segment less.
      i--;
      // If we're down to 0, special-case to root.
      if (i == 0)
      {
        parent = getRoot();
        return theFile;
      }
      continue;
    }
  }

  // We shouldn't get here.
  return VFS::invalidFile();
}

void Filesystem::cacheInsert(String canonicalPath, File* parent)
{
  File *pFile = new File(parent);
  m_Cache.insert(canonicalPath, pFile);
}
