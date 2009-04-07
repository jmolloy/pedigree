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

#include <Module.h>

#include <processor/Processor.h>
#include <process/Process.h>

typedef Tree<size_t,FileDescriptor*> FdMap;

int posix_pipe(int filedes[2])
{
  NOTICE("pipe");

  FdMap& fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

  size_t readFd = Processor::information().getCurrentThread()->getParent()->nextFd();
  size_t writeFd = Processor::information().getCurrentThread()->getParent()->nextFd();

  filedes[0] = readFd;
  filedes[1] = writeFd;

  File* p = new Pipe(String("Anonymous pipe"), 0, 0, 0, 0, 0, 0, 0, true);

  // create the file descriptor for both
  FileDescriptor* read = new FileDescriptor;
  read->file = p;
  read->offset = 0;
  fdMap.insert(readFd, read);
  p->increaseRefCount(false);

  FileDescriptor* write = new FileDescriptor;
  write->file = p;
  write->offset = 0;
  write->flflags = O_WRONLY;
  fdMap.insert(writeFd, write);
  p->increaseRefCount(true);

  return 0;
}
