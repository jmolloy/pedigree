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
#ifndef CONSOLE_H
#define CONSOLE_H

#include <vfs/VFS.h>
#include <vfs/Filesystem.h>
#include <utilities/RequestQueue.h>
#include <utilities/Vector.h>

#define CONSOLE_READ    1
#define CONSOLE_WRITE   2
#define CONSOLE_SETATTR 3
#define CONSOLE_GETATTR 4
#define CONSOLE_GETROWS 5
#define CONSOLE_GETCOLS 6
#define CONSOLE_DATA_AVAILABLE 7

/** This class provides a way for consoles (TTYs) to be created to interact with applications.

    registerConsole is called by a class that subclasses RequestQueue. getConsole returns a File that forwards
    any read/write requests straight to that RequestQueue backend.

    read: request with p1: CONSOLE_READ, p2: param, p3: requested size, p4: buffer.
    write: request with p1: CONSOLE_WRITE, p2: param, p3: size of buffer, p4: buffer.
*/
class ConsoleManager : public Filesystem
{
public:
  ConsoleManager();

  virtual ~ConsoleManager();

  static ConsoleManager &instance();

  //
  // ConsoleManager interface.
  //
  bool registerConsole(String consoleName, RequestQueue *backEnd, uintptr_t param);

  File* getConsole(String consoleName);

  bool isConsole(File* file);

  void setAttributes(File* file, bool echo, bool echoNewlines, bool echoBackspace, bool nlCausesCr);
  void getAttributes(File* file, bool *echo, bool *echoNewlines, bool *echoBackspace, bool *nlCausesCr);
  int  getCols(File* file);
  int  getRows(File* file);
  bool hasDataAvailable(File* file);

  //
  // Filesystem interface.
  //

  virtual bool initialise(Disk *pDisk)
    {return false;}
  virtual File* getRoot()
  {return VFS::invalidFile();}
  virtual String getVolumeLabel()
  {return String("consolemanager");}
  virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
  virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
  virtual void truncate(File *pFile)
  {}
  virtual void fileAttributeChanged(File *pFile)
  {}
  virtual File* getDirectoryChild(File *pFile, size_t n)
  {return VFS::invalidFile();}

protected:
  virtual bool createFile(File* parent, String filename, uint32_t mask)
  {return false;}
  virtual bool createDirectory(File* parent, String filename)
  {return false;}
  virtual bool createSymlink(File* parent, String filename, String value)
  {return false;}
  virtual bool remove(File* parent, File* file)
  {return false;}

private:
  struct Console
  {
    String name;
    RequestQueue *backEnd;
    uintptr_t param;
  };

  Vector<Console*> m_Consoles;
  static ConsoleManager m_Instance;
};

#endif
