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

#include "pipe-syscalls.h"
#include "file-syscalls.h"
#include <vfs/VFS.h>
#include <Module.h>

#include <processor/Processor.h>
#include <process/Process.h>

PipeManager PipeManager::m_Instance;

PipeFile::PipeFile() : File()
{};

PipeFile::PipeFile(const File &file) : File(file)
{};

//PipeFile::PipeFile(File* file) : File(file)
//{};

PipeFile::PipeFile(String name, Time accessedTime, Time modifiedTime, Time creationTime,
                    uintptr_t inode, bool isSymlink, bool isDirectory, class Filesystem *pFs, size_t size)
                    :
                    File(name, accessedTime, modifiedTime, creationTime, inode, isSymlink, isDirectory, pFs, size)
{};

PipeFile::~PipeFile()
{
  PipeManager::instance().fileEnds(reinterpret_cast<File*>(this));
}


PipeManager::PipeManager() :
  m_Pipes()
{
}

PipeManager::~PipeManager()
{
}

PipeManager &PipeManager::instance()
{
  return m_Instance;
}

File* PipeManager::getPipe()
{
  /// \todo Figure out how to free this later
  Pipe* pipe = new Pipe;
  size_t n = m_Pipes.count();
  m_Pipes.pushBack(pipe);
  File* ret = new File(String("pipe"), 0, 0, 0, n + 0xdcba0000, false, false, this, 0);
  pipe->nRefs = 1;
  pipe->original = ret;
  return ret;
}

bool PipeManager::isPipe(File* file)
{
  return ((file->getInode() & 0xFFFF0000) == 0xdcba0000);
}

uint64_t PipeManager::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  NOTICE("PipeManager::read");
  
  /// \todo Sanity checking.
  Pipe* pipe = m_Pipes[pFile->getInode()-0xdcba0000];
  
  if(pipe->fifo.getSize() == 0)
  {
    return 0;
  }
  
  // read from the front of the buffer
  uint64_t nBytes = size;
  if(nBytes > pipe->fifo.getSize())
    nBytes = pipe->fifo.getSize();
  
  // copy the data
  uintptr_t front = pipe->fifo.getBuffer();
  memcpy(reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(front), nBytes);
  
  // remove from the queue
  pipe->fifo.remove(0, nBytes);
  
  // all done
  return nBytes;
}

uint64_t PipeManager::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  NOTICE("PipeManager::write");
  
  /// \todo Sanity checking.
  Pipe* pipe = m_Pipes[pFile->getInode()-0xdcba0000];
  pipe->fifo.append(buffer, size);
  return size;
}

typedef Tree<size_t,FileDescriptor*> FdMap;

int posix_pipe(int filedes[2])
{
  NOTICE("pipe");
  
  FdMap& fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();
  
  size_t readFd = Processor::information().getCurrentThread()->getParent()->nextFd();
  size_t writeFd = Processor::information().getCurrentThread()->getParent()->nextFd();
  
  filedes[0] = readFd;
  filedes[1] = writeFd;
  
  File* p = PipeManager::instance().getPipe();
  
  // create the file descriptor for both
  FileDescriptor* read = new FileDescriptor;
  read->file = PipeManager::instance().copyPipe(p);
  read->offset = 0;
  fdMap.insert(readFd, read);
  
  FileDescriptor* write = new FileDescriptor;
  write->file = PipeManager::instance().copyPipe(p);
  write->offset = 0;
  fdMap.insert(writeFd, write);
  
  return 0;
}
