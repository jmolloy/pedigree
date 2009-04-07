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
#ifndef VFS_H
#define VFS_H

#include <processor/types.h>
#include <utilities/List.h>
#include <utilities/String.h>
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
  static VFS &instance();

  /** Mounts a Disk device as the alias "alias".
      If alias is zero-length, the Filesystem is asked for its preferred name
      (usually a volume name of some sort), and returned in "alias" */
  bool mount(Disk *pDisk, String &alias);

  /** Adds an alias to an existing filesystem.
   *\param pFs The filesystem to add an alias for.
   *\param pAlias The alias to add. */
  void addAlias(Filesystem *pFs, String alias);
  void addAlias(String oldAlias, String newAlias);

  /** Gets a unique alias for a filesystem. */
  String getUniqueAlias(String alias);

  /** Does a given alias exist? */
  bool aliasExists(String alias);

  /** Removes an alias from a filesystem. If no aliases remain for that filesystem,
   *  the filesystem is destroyed.
   *\param pAlias The alias to remove. */
  void removeAlias(String alias);

  /** Removes all aliases from a filesystem - the filesystem is destroyed.
   *\param pFs The filesystem to destroy. */
  void removeAllAliases(Filesystem *pFs);

  /** Looks up the Filesystem from a given alias.
   *\param pAlias The alias to search for.
   *\return The filesystem aliased by pAlias or 0 if none found. */
  Filesystem *lookupFilesystem(String alias);

  /** Attempts to obtain a File for a specific path. */
  File *find(String path, File *pStartNode=0);

  /** Attempts to create a file. */
  bool createFile(String path, uint32_t mask, File *pStartNode=0);

  /** Attempts to create a directory. */
  bool createDirectory(String path, File *pStartNode=0);

  /** Attempts to create a symlink. */
  bool createSymlink(String path, String value, File *pStartNode=0);

  /** Attempts to remove a file/directory/symlink. WILL FAIL IF DIRECTORY NOT EMPTY */
  bool remove(String path, File *pStartNode=0);

  /** Adds a filesystem probe callback - this is called when a device is mounted. */
  void addProbeCallback(Filesystem::ProbeCallback callback);

private:
  /** The static instance object. */
  static VFS m_Instance;

  /** A static File object representing an invalid file */
  static File* m_EmptyFile;

  /** Structure matching aliases to filesystems.
   * \todo Use a proper Map class for this. */
  struct Alias
  {
    Alias() : alias(), fs(0) {}
    String alias;
    Filesystem *fs;
  private:
    Alias(const Alias&);
    void operator =(const Alias&);
  };
  List<Alias*> m_Aliases;

  List<Filesystem::ProbeCallback*> m_ProbeCallbacks;
};

#endif
