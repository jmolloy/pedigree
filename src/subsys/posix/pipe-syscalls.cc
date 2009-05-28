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
#include <vfs/Pipe.h>

#include <Subsystem.h>
#include <PosixSubsystem.h>

#include <Module.h>

#include <processor/Processor.h>
#include <process/Process.h>

typedef Tree<size_t,FileDescriptor*> FdMap;

int posix_pipe(int filedes[2])
{
  NOTICE("pipe");

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  size_t readFd = pSubsystem->getFd();
  size_t writeFd = pSubsystem->getFd();

  filedes[0] = readFd;
  filedes[1] = writeFd;

  File* p = new Pipe(String("Anonymous pipe"), 0, 0, 0, 0, 0, 0, 0, true);

  // create the file descriptor for both
  FileDescriptor* read = new FileDescriptor;
  read->file = p;
  read->offset = 0;
  pSubsystem->addFileDescriptor(readFd, read);
  // p->increaseRefCount(false);

  FileDescriptor* write = new FileDescriptor;
  write->file = p;
  write->offset = 0;
  write->flflags = O_WRONLY;
  pSubsystem->addFileDescriptor(writeFd, write);
  // p->increaseRefCount(true);

  return 0;
}
