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

#include "PosixSyscallManager.h"
#include <processor/SyscallManager.h>
#include <processor/Processor.h>
#include <process/Scheduler.h>
#include <Log.h>
#include "syscallNumbers.h"
#include "file-syscalls.h"
#include "system-syscalls.h"
#include "console-syscalls.h"

PosixSyscallManager::PosixSyscallManager()
{
  magic = 0x4567;
}

PosixSyscallManager::~PosixSyscallManager()
{
}

void PosixSyscallManager::initialise()
{
  SyscallHandler *sh = (SyscallHandler*)this;
  SyscallManager::instance().registerSyscallHandler(posix, this);
}

uintptr_t PosixSyscallManager::call(uintptr_t function, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
  // if (function >= serviceEnd)
  // {
  //   ERROR("PosixSyscallManager: invalid function called: " << Dec << static_cast<int>(function));
  //   return 0;
  // }
  return SyscallManager::instance().syscall(posix, function, p1, p2, p3, p4, p5);
}

uintptr_t PosixSyscallManager::syscall(SyscallState &state)
{
  uintptr_t p1 = state.getSyscallParameter(0);
  uintptr_t p2 = state.getSyscallParameter(1);
  uintptr_t p3 = state.getSyscallParameter(2);
  uintptr_t p4 = state.getSyscallParameter(3);
  uintptr_t p5 = state.getSyscallParameter(4);

  NOTICE("[" << Processor::information().getCurrentThread()->getParent()->getId() << "] : " << state.getSyscallNumber());

  // We're interruptible.
  Processor::setInterrupts(true);

  switch (state.getSyscallNumber())
  {
    case POSIX_OPEN:
      return posix_open(reinterpret_cast<const char*>(p1), p2, p3);
    case POSIX_WRITE:
      return posix_write(p1, reinterpret_cast<char*>(p2), p3);
    case POSIX_READ:
      return posix_read(p1, reinterpret_cast<char*>(p2), p3);
    case POSIX_CLOSE:
      return posix_close(p1);
    case POSIX_SBRK:
      return posix_sbrk(p1);
    case POSIX_FORK:
      return posix_fork(state);
    case POSIX_EXECVE:
      return posix_execve(reinterpret_cast<const char*>(p1), reinterpret_cast<const char**>(p2), reinterpret_cast<const char**>(p3), state);
    case POSIX_WAITPID:
      return posix_waitpid(p1, reinterpret_cast<int*> (p2), p3);
    case POSIX_EXIT:
      return posix_exit(p1);
    case POSIX_OPENDIR:
      return posix_opendir(reinterpret_cast<const char*>(p1), reinterpret_cast<dirent*>(p2));
    case POSIX_READDIR:
      return posix_readdir(p1, reinterpret_cast<dirent*>(p2));
    case POSIX_REWINDDIR:
      posix_rewinddir(p1, reinterpret_cast<dirent*>(p2));
      return 0;
    case POSIX_CLOSEDIR:
      return posix_closedir(p1);
    case POSIX_TCGETATTR:
      return posix_tcgetattr(p1, reinterpret_cast<struct termios*>(p2));
    case POSIX_TCSETATTR:
      return posix_tcsetattr(p1, p2, reinterpret_cast<struct termios*>(p3));
    case POSIX_IOCTL:
      return posix_ioctl(p1, p2, reinterpret_cast<void*>(p3));
    case POSIX_STAT:
      return posix_stat(reinterpret_cast<const char*>(p1), reinterpret_cast<struct stat*>(p2));
    case POSIX_FSTAT:
      return posix_fstat(p1, reinterpret_cast<struct stat*>(p2));
    case POSIX_GETPID:
      return posix_getpid();
    case POSIX_CHDIR:
      return posix_chdir(reinterpret_cast<const char*>(p1));
    case POSIX_SELECT:
      return posix_select(static_cast<int>(p1), reinterpret_cast<struct fd_set*>(p2), reinterpret_cast<struct fd_set*>(p3),
                          reinterpret_cast<struct fd_set*>(p4), reinterpret_cast<struct timeval*>(p5));
    default: ERROR ("PosixSyscallManager: invalid syscall received: " << Dec << state.getSyscallNumber()); return 0;
  }
}
