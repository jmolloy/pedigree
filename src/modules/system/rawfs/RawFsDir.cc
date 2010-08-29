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

#include "RawFsDir.h"
#include "RawFs.h"

RawFsDir::RawFsDir(String name, RawFs *pFs, File *pParent) :
    Directory(name,
	      0 /* Accessed time */,
	      0 /* Modified time */,
	      0 /* Creation time */,
	      0 /* Inode number */,
	      static_cast<Filesystem*>(pFs),
	      0 /* Size */,
	      pParent)
{
}

void RawFsDir::addEntry(File *pEntry)
{
    m_Cache.insert(pEntry->getName(), pEntry);
}

void RawFsDir::removeRecursive()
{
    /// \todo Leaky.
    NOTICE("rawfs: removing '" << getName() << "'");
    m_Cache.clear();
}
