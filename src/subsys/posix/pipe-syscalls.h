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

/** A special version of File which talks to PipeManager every time it is freed */
class PipeFile : public File
{
  public:
    /** Constructor, creates an invalid file. */
    PipeFile();
    PipeFile(const File &file);
//    PipeFile(File* file);

    /** Constructor, should be called only by a Filesystem. */
    PipeFile(String name, Time accessedTime, Time modifiedTime, Time creationTime,
         uintptr_t inode, bool isSymlink, bool isDirectory, class Filesystem *pFs, size_t size);
    /** Destructor - removes a reference from the PipeManager's internal list. */
    virtual ~PipeFile();
};

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

  File* copyPipe(File* file)
  {
    if(isPipe(file))
    {
      size_t n = file->getInode() - 0xdcba0000;
      m_Pipes[n]->nRefs++;

      return new PipeFile(*file);
    }
    else
      return new File(*file);
  }

  void fileEnds(File* f)
  {
    size_t n = f->getInode() - 0xdcba0000;
    if(--(m_Pipes[n]->nRefs) == 1)
    {
      size_t i = 0;
      for(Vector<Pipe*>::Iterator it = m_Pipes.begin(); it != m_Pipes.end(); it++, i++)
      {
        if(i == n)
        {
          Pipe* pipe = m_Pipes[n];
          m_Pipes.erase(it);

          if (pipe->original->shouldDelete()) delete pipe->original;
          delete pipe;
          break;
        }
      }
    }
  }

  //
  // Filesystem interface.
  //

  virtual bool initialise(Disk *pDisk)
    {return false;}
  virtual File* getRoot()
  {return VFS::invalidFile();}
  virtual String getVolumeLabel()
  {return String("pipemanager");}
  virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
  virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
  virtual void truncate(File *pFile)
  {}
  virtual void fileAttributeChanged(File *pFile)
  {}
  virtual void cacheDirectoryContents(File *pFile)
  {}

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
    Pipe() : fifo(), nRefs(0), original(0) {};
    Pipe(const Pipe& pipe) : fifo(pipe.fifo), nRefs(pipe.nRefs), original(new File(*(pipe.original))) {};
    ~Pipe() {};

    TcpBuffer fifo;

    size_t nRefs;
    File* original;

    Pipe& operator = (const Pipe& pipe)
    {
      fifo = pipe.fifo;
      nRefs = pipe.nRefs;
      original = new File(*(pipe.original));

      return *this;
    }
  };

  Vector<Pipe*> m_Pipes;
  static PipeManager m_Instance;
};

#endif
