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
#include <processor/Processor.h>
#include <process/Process.h>
#include <utilities/Tree.h>
#include <vfs/File.h>
#include <vfs/VFS.h>
#include <console/Console.h>

#include "file-syscalls.h"
#include "console-syscalls.h"

typedef Tree<size_t,void*> FdMap;

int posix_tcgetattr(int fd, struct termios *p)
{
  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

  FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));
  if (!pFd)
  {
    // Error - no such file descriptor.
    return -1;
  }

  if (!ConsoleManager::instance().isConsole(pFd->file))
  {
    // Error - not a TTY.
    return -1;
  }

  bool echo, echoNewlines, echoBackspace, nlCausesCr, mapNlToCrIn, mapCrToNlIn;
  ConsoleManager::instance().getAttributes(pFd->file, &echo, &echoNewlines, &echoBackspace, &nlCausesCr, &mapNlToCrIn, &mapCrToNlIn);
  NOTICE("nlToCr on retu: " << mapNlToCrIn);
  memset(p->c_cc, 0, NCCS*sizeof(cc_t));
  p->c_cc[VEOL] = '\n';

  p->c_iflag = ((mapNlToCrIn)?INLCR:0) | ((mapCrToNlIn)?ICRNL:0);
  p->c_oflag = (nlCausesCr)?ONLRET:0;
  p->c_cflag = 0;
  p->c_lflag = ((echo)?ECHO:0) | ((echoNewlines)?ECHONL:0) | ((echoBackspace)?ECHOE:0);

  return 0;
}

int posix_tcsetattr(int fd, int optional_actions, struct termios *p)
{
  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

  FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));
  if (!pFd)
  {
    // Error - no such file descriptor.
    return -1;
  }

  if (!ConsoleManager::instance().isConsole(pFd->file))
  {
    // Error - not a TTY.
    return -1;
  }

  /// \todo Sanity checks.
  NOTICE("Setattributes: iflag: " << p->c_iflag << ", oflag: " << p->c_oflag);
  ConsoleManager::instance().setAttributes(pFd->file, p->c_lflag&ECHO, p->c_lflag&ECHONL, p->c_lflag&ECHOE, p->c_oflag&ONLRET, p->c_iflag&INLCR, p->c_iflag&ICRNL);

  return 0;
}

int console_getwinsize(File* file, winsize_t *buf)
{
  if (!ConsoleManager::instance().isConsole(file))
  {
    // Error - not a TTY.
    return -1;
  }
  buf->ws_row = ConsoleManager::instance().getRows(file);
  buf->ws_col = ConsoleManager::instance().getCols(file);
  return 0;
}
