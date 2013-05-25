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

#include <Subsystem.h>
#include <PosixSubsystem.h>

#include "file-syscalls.h"
#include "console-syscalls.h"

typedef Tree<size_t,FileDescriptor*> FdMap;

int posix_tcgetattr(int fd, struct termios *p)
{
  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  FileDescriptor *pFd = pSubsystem->getFileDescriptor(fd);
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

  size_t flags;
  ConsoleManager::instance().getAttributes(pFd->file, &flags);

  memset(p->c_cc, 0, NCCS*sizeof(cc_t));
  p->c_cc[VEOL] = '\n';

  p->c_iflag = ((flags&ConsoleManager::IMapNLToCR)?INLCR:0) | 
               ((flags&ConsoleManager::IMapCRToNL)?ICRNL:0) |
               ((flags&ConsoleManager::IIgnoreCR)?IGNCR:0) |
               ((flags&ConsoleManager::IStripToSevenBits)?ISTRIP:0);
  p->c_oflag = ((flags&ConsoleManager::OPostProcess)?OPOST:0) |
               ((flags&ConsoleManager::OMapCRToNL)?OCRNL:0) |
               ((flags&ConsoleManager::OMapNLToCRNL)?ONLCR:0) |
               ((flags&ConsoleManager::ONLCausesCR)?ONLRET:0);
  p->c_cflag = 0;
  p->c_lflag = ((flags&ConsoleManager::LEcho)?ECHO:0) |
      ((flags&ConsoleManager::LEchoErase)?ECHOE:0) |
      ((flags&ConsoleManager::LEchoKill)?ECHOK:0) |
      ((flags&ConsoleManager::LEchoNewline)?ECHONL:0) |
      ((flags&ConsoleManager::LCookedMode)?ICANON:0);

  F_NOTICE("posix_tcgetattr returns {c_iflag=" << p->c_iflag << ", c_oflag=" << p->c_oflag << ", c_lflag=" << p->c_lflag << "} )");
  return 0;
}

int posix_tcsetattr(int fd, int optional_actions, struct termios *p)
{
  F_NOTICE("posix_tcsetattr(" << fd << ", " << optional_actions << ", {c_iflag=" << p->c_iflag << ", c_oflag=" << p->c_oflag << ", c_lflag=" << p->c_lflag << "} )");

  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  FileDescriptor *pFd = pSubsystem->getFileDescriptor(fd);
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

  size_t flags = 0;
  if (p->c_iflag&INLCR)  flags |= ConsoleManager::IMapNLToCR;
  if (p->c_iflag&ICRNL)  flags |= ConsoleManager::IMapCRToNL;
  if (p->c_iflag&IGNCR)  flags |= ConsoleManager::IIgnoreCR;
  if (p->c_iflag&ISTRIP) flags |= ConsoleManager::IStripToSevenBits;
  if (p->c_oflag&OPOST)  flags |= ConsoleManager::OPostProcess;
  if (p->c_oflag&OCRNL)  flags |= ConsoleManager::OMapCRToNL;
  if (p->c_oflag&ONLCR)  flags |= ConsoleManager::OMapNLToCRNL;
  if (p->c_oflag&ONLRET) flags |= ConsoleManager::ONLCausesCR;
  if (p->c_lflag&ECHO)   flags |= ConsoleManager::LEcho;
  if (p->c_lflag&ECHOE)  flags |= ConsoleManager::LEchoErase;
  if (p->c_lflag&ECHOK)  flags |= ConsoleManager::LEchoKill;
  if (p->c_lflag&ECHONL) flags |= ConsoleManager::LEchoNewline;
  if (p->c_lflag&ICANON) flags |= ConsoleManager::LCookedMode;
  NOTICE("TCSETATTR: " << Hex << flags);
  /// \todo Sanity checks.
  ConsoleManager::instance().setAttributes(pFd->file, flags);

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

int console_flush(File *file, void *what)
{
  if (!ConsoleManager::instance().isConsole(file))
  {
    // Error - not a TTY.
    return -1;
  }

  /// \todo handle 'what' parameter
  ConsoleManager::instance().flush(file);
  return 0;
}

