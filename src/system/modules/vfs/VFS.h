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

#include <processor/types.h>
#include "Filesystem.h"

/** This class implements a virtual file system.
 * 
 * The pedigree VFS is structured in a similar way to windows' - every filesystem
 * is identified by a unique name and accessed thus:
 * 
 * myfs:/mydir/myfile
 * 
 * No UNIX-style mounting of filesystems inside filesystems is possible.
 * A filesystem may be referred to by multiple names - a reference count is 
 * maintained by the filesystem - when no aliases point to it, it is unmounted totally.
 * 
 * The 'root' filesystem - that is the FS with system data on, is visible by the alias
 * 'root', thus; 'root:/System/Boot/kernel' could be used to access the kernel image.
 */
class VFS
{
public:
  /** Constructor */
  VFS();
  /** Destructor */
  ~VFS();
  
  /** Returns the singleton VFS instance. */
  VFS &instance();
  
  /** Registers a filesystem - initially the FS is referred to by only the given alias.
   *\param pFs The filesystem to register.
   *\param pAlias The alias by which it is known.
   *\return True on success, false on failure.
   */
  bool registerFil
  
};