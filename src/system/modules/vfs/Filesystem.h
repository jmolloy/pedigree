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
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <processor/types.h>
#include <utilities/String.h>
#include <machine/Disk.h>
#include <vfs/File.h>

/** This class provides the abstract skeleton that all filesystems must implement.
 */
class Filesystem
{
/** VFS can access nAliases */
friend class VFS;

public:
  //
  // Public interface
  //

  /** Constructor - creates a blank object. */
  Filesystem();
  /** Destructor */
  virtual ~Filesystem() {}

  /** Populates this filesystem with data from the given Disk device.
   * \return true on success, false on failure. */
  virtual bool initialise(Disk *pDisk) =0;

  /** Type of the probing callback given to the VFS.
      Probe function - if this filesystem is found on the given Disk 
      device, create a new instance of it and return that. Else return 0. */
  typedef Filesystem *(*ProbeCallback)(Disk *);

  /** Attempt to find a file or directory in this filesystem.
      \param path The path to the file, in UTF-8 format, without filesystem identifier
                  (e.g. not "root:/file", but "/file").
      \return The file if one was found, or 0 otherwise or if there was an error.
  */
  virtual File find(String path);

  /** Returns the root filesystem node. */
  virtual File getRoot() =0;

  /** Returns a string identifying the volume label. */
  virtual String getVolumeLabel() =0;
  
  /** Creates a file on the filesystem - fails if the file's parent directory does not exist. */
  bool createFile(String path);

  /** Creates a directory on the filesystem. Fails if the dir's parent directory does not exist. */
  bool createDirectory(String path);

  /** Creates a symlink on the filesystem, with the given value. */
  bool createSymlink(String path, String value);

  /** Removes a file, directory or symlink. WILL FAIL IF DIRECTORY NOT EMPTY! */
  bool remove(String path);

  //
  // File interface.
  //

  /** A File calls this to pass up a read request. */
  virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer) =0;
  
  /** A File calls this to pass up a write request. */
  virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer) =0;

  /** A File calls this to pass up a request to truncate a file - remove all data. */
  virtual void truncate(File *pFile) =0;

  /** A File calls this to propagate a change in a file attribute. */
  virtual void fileAttributeChanged(File *pFile) =0;

  /** A File calls this to get the n'th child of a directory. */
  virtual File getDirectoryChild(File *pFile, size_t n) =0;

protected:
  /** createFile calls this after it has parsed the string path. */
  virtual bool createFile(File parent, String filename) =0;
  /** createDirectory calls this after it has parsed the string path. */
  virtual bool createDirectory(File parent, String filename) =0;
  /** createSymlink calls this after it has parsed the string path. */
  virtual bool createSymlink(File parent, String filename, String value) =0;
  /** remove() calls this after it has parsed the string path. */
  virtual bool remove(File parent, File file) =0;

private:
  /** Accessed by VFS */
  size_t nAliases;
};

#endif
