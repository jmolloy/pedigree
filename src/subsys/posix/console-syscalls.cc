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
#include <syscallError.h>

#include <Subsystem.h>
#include <PosixSubsystem.h>
#include <PosixProcess.h>

#include "file-syscalls.h"
#include "console-syscalls.h"

typedef Tree<size_t,FileDescriptor*> FdMap;

class PosixTerminalEvent : public Event
{
    public:
        PosixTerminalEvent() :
            Event(0, false), pGroup(0), pConsole(0)
        {}
        PosixTerminalEvent(uintptr_t handlerAddress, ProcessGroup *grp, ConsoleFile *tty, size_t specificNestingLevel=~0UL) :
            Event(handlerAddress, false, specificNestingLevel),
            pGroup(grp), pConsole(tty)
        {
        }
        virtual ~PosixTerminalEvent()
        {
            // Remove us from the console if needed.
            if(pConsole->getEvent() == this)
            {
                pConsole->setEvent(0);
            }
        }

        virtual size_t serialize(uint8_t *pBuffer)
        {
            size_t eventNumber = EventNumbers::TerminalEvent;
            size_t offset = 0;
            memcpy(pBuffer + offset, &eventNumber, sizeof(eventNumber));
            offset += sizeof(eventNumber);
            memcpy(pBuffer + offset, &pGroup, sizeof(pGroup));
            offset += sizeof(pGroup);
            memcpy(pBuffer + offset, &pConsole, sizeof(pConsole));
            offset += sizeof(pConsole);
            return offset;
        }

        static bool unserialize(uint8_t *pBuffer, Event &event)
        {
            PosixTerminalEvent &t = static_cast<PosixTerminalEvent&>(event);
            if(Event::getEventType(pBuffer) != EventNumbers::TerminalEvent)
                return false;
            size_t offset = sizeof(size_t);
            memcpy(&t.pGroup, pBuffer + offset, sizeof(t.pGroup));
            offset += sizeof(t.pGroup);
            memcpy(&t.pConsole, pBuffer + offset, sizeof(t.pConsole));
            return true;
        }

        virtual ProcessGroup *getGroup() const
        {
            return pGroup;
        }

        virtual ConsoleFile *getConsole() const
        {
            return pConsole;
        }

        virtual size_t getNumber()
        {
            return EventNumbers::TerminalEvent;
        }

        virtual bool isDeleteable()
        {
            return false;
        }

    private:
        ProcessGroup *pGroup;
        ConsoleFile *pConsole;
};

void terminalEventHandler(uintptr_t serializeBuffer)
{
    PosixTerminalEvent evt;
    if(!PosixTerminalEvent::unserialize(reinterpret_cast<uint8_t *>(serializeBuffer), evt))
    {
        return;
    }

    ConsoleFile *pConsole = evt.getConsole();
    ProcessGroup *pGroup = evt.getGroup();

    // Grab the character which caused the event.
    char which = pConsole->getLast();

    // Grab the special characters - we'll use these to figure out what we hit.
    char specialChars[NCCS];
    pConsole->getControlCharacters(specialChars);

    // Identify what happened.
    Subsystem::ExceptionType what = Subsystem::Other;
    if(which == specialChars[VINTR])
        what = Subsystem::Interrupt;
    else if(which == specialChars[VQUIT])
        what = Subsystem::Quit;
    else if(which == specialChars[VSUSP])
        what = Subsystem::Stop;

    // Dummy interrupt state for Process::threadException
    InterruptState *state = 0;

    // Send to each process.
    if(what != Subsystem::Other)
    {
        for(List<PosixProcess*>::Iterator it = pGroup->Members.begin();
            it != pGroup->Members.end();
            ++it)
        {
            PosixProcess *pProcess = *it;
            PosixSubsystem *pSubsystem = static_cast<PosixSubsystem*>(pProcess->getSubsystem());
            pSubsystem->threadException(pProcess->getThread(0), what, *state);
        }
    }

    // We have finished handling this event.
    pConsole->eventComplete();
}

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
      ((flags&ConsoleManager::LCookedMode)?ICANON:0) |
      ((flags&ConsoleManager::LGenerateEvent)?ISIG:0);

  char controlChars[MAX_CONTROL_CHAR] = {0};
  ConsoleManager::instance().getControlChars(pFd->file, controlChars);

  // c_cc is of type cc_t, but we don't want to expose that type to ConsoleManager.
  // By doing this conversion, we can use whatever type we like in the kernel.
  for(size_t i = 0; i < NCCS; ++i)
      p->c_cc[i] = controlChars[i];

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
  if (p->c_lflag&ISIG)   flags |= ConsoleManager::LGenerateEvent;
  NOTICE("TCSETATTR: " << Hex << flags);
  /// \todo Sanity checks.
  ConsoleManager::instance().setAttributes(pFd->file, flags);

  char controlChars[MAX_CONTROL_CHAR] = {0};
  for(size_t i = 0; i < NCCS; ++i)
    controlChars[i] = p->c_cc[i];
  ConsoleManager::instance().setControlChars(pFd->file, controlChars);

  return 0;
}

int console_getwinsize(File* file, winsize_t *buf)
{
  if (!ConsoleManager::instance().isConsole(file))
  {
    // Error - not a TTY.
    return -1;
  }

  return ConsoleManager::instance().getWindowSize(file, &buf->ws_row, &buf->ws_col);
}

int console_setwinsize(File *file, const winsize_t *buf)
{
  if (!ConsoleManager::instance().isConsole(file))
  {
    // Error - not a TTY.
    return -1;
  }

  /// \todo Send SIGWINCH to foreground process group (once we have one)
  return ConsoleManager::instance().setWindowSize(file, buf->ws_row, buf->ws_col);
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

void console_ptsname(int fd, char *buf)
{
  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return;
  }

  FileDescriptor *pFd = pSubsystem->getFileDescriptor(fd);
  if (!pFd)
  {
    // Error - no such file descriptor.
    return;
  }

  if (!ConsoleManager::instance().isConsole(pFd->file))
  {
    // Error - not a TTY.
    return;
  }

  File *slave = pFd->file;
  if(ConsoleManager::instance().isMasterConsole(slave))
  {
    slave = ConsoleManager::instance().getOther(pFd->file);
  }

  sprintf(buf, "/dev/%s", static_cast<const char *>(slave->getName()));
}

void console_ttyname(int fd, char *buf)
{
  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return;
  }

  FileDescriptor *pFd = pSubsystem->getFileDescriptor(fd);
  if (!pFd)
  {
    // Error - no such file descriptor.
    return;
  }

  if (!ConsoleManager::instance().isConsole(pFd->file))
  {
    // Error - not a TTY.
    return;
  }

  File *master = pFd->file;
  if(!ConsoleManager::instance().isMasterConsole(master))
  {
    master = ConsoleManager::instance().getOther(pFd->file);
  }

  sprintf(buf, "/dev/%s", static_cast<const char *>(master->getName()));
}

int posix_tcsetpgrp(int fd, pid_t pgid_id)
{
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
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }

    if ((!pProcess->getCtty()) || (pProcess->getCtty() != pFd->file) || (!ConsoleManager::instance().isConsole(pFd->file)))
    {
        SYSCALL_ERROR(NotAConsole);
        return -1;
    }

    // Find the group ID.
    ProcessGroup *pGroup = 0;
    for(size_t i = 0; i < Scheduler::instance().getNumProcesses(); i++)
    {
        Process *p = Scheduler::instance().getProcess(i);
        if(p->getType() == Process::Posix)
        {
            PosixProcess *pPosix = static_cast<PosixProcess *>(p);
            pGroup = pPosix->getProcessGroup();
            if(pGroup && (pGroup->processGroupId == pgid_id))
            {
                break;
            }
            else
            {
                pGroup = 0;
            }
        }
    }

    if(!pGroup)
    {
        SYSCALL_ERROR(PermissionDenied);
        return -1;
    }

    // Okay, we have a group. Create a PosixTerminalEvent with the relevant information.
    ConsoleFile *pConsole = static_cast<ConsoleFile *>(pProcess->getCtty());
    PosixTerminalEvent *pEvent = new PosixTerminalEvent(reinterpret_cast<uintptr_t>(terminalEventHandler), pGroup, pConsole);

    // Remove any existing event that might be on the terminal.
    if(pConsole->getEvent())
    {
        PosixTerminalEvent *pOldEvent = static_cast<PosixTerminalEvent *>(pConsole->getEvent());
        pConsole->setEvent(0);
        delete pOldEvent;
    }

    // Set as the new event - we are now the foreground process!
    /// \todo This doesn't work for SIGTTIN and SIGTTOU...
    pConsole->setEvent(pEvent);

    return 0;
}

pid_t posix_tcgetpgrp(int fd)
{
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
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }

    if ((!pProcess->getCtty()) || (pProcess->getCtty() != pFd->file) || (!ConsoleManager::instance().isConsole(pFd->file)))
    {
        SYSCALL_ERROR(NotAConsole);
        return -1;
    }

    // Remove any existing event that might be on the terminal.
    ConsoleFile *pConsole = static_cast<ConsoleFile *>(pProcess->getCtty());
    if(pConsole->getEvent())
    {
        PosixTerminalEvent *pEvent = static_cast<PosixTerminalEvent*>(pConsole->getEvent());
        return pEvent->getGroup()->processGroupId;
    }
    else
    {
        return (pid_t) ~0;
    }
}

