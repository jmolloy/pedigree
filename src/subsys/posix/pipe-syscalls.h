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

#ifndef PIPE_SYSCALLS_H
#define PIPE_SYSCALLS_H

#include <vfs/VFS.h>
#include <vfs/File.h>
#include <vfs/Filesystem.h>
#include <network/TcpMisc.h>

int posix_pipe(int filedes[2]);

/** This class provides a way for pipes to be created and accessed by user applications */
class PipeManager : public Filesystem
{
public:
  PipeManager();

  virtual ~PipeManager();

  static PipeManager &instance();

  //
  // PipeManager interface.
  //
  
  File* getPipe();
  
  bool isPipe(File* file);
  
  /*bool registerConsole(String consoleName, RequestQueue *backEnd, uintptr_t param);

  File* getConsole(String consoleName);

  bool isConsole(File* file);

  void setAttributes(File* file, bool echo, bool echoNewlines, bool echoBackspace, bool nlCausesCr);
  void getAttributes(File* file, bool *echo, bool *echoNewlines, bool *echoBackspace, bool *nlCausesCr);
  int  getCols(File* file);
  int  getRows(File* file);
  bool hasDataAvailable(File* file);*/

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
  struct Pipe
  {
    TcpBuffer fifo;
  };

  Vector<Pipe*> m_Pipes;
  static PipeManager m_Instance;
};

#endif
