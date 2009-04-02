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

#include <processor/SyscallManager.h>
#include <processor/Processor.h>
#include <process/Scheduler.h>
#include <Log.h>

#include "PosixSyscallManager.h"
#include "syscallNumbers.h"
#include "file-syscalls.h"
#include "system-syscalls.h"
#include "console-syscalls.h"
#include "net-syscalls.h"
#include "pipe-syscalls.h"

PosixSyscallManager::PosixSyscallManager()
{
}

PosixSyscallManager::~PosixSyscallManager()
{
}

void PosixSyscallManager::initialise()
{
  SyscallManager::instance().registerSyscallHandler(posix, this);
}

uintptr_t PosixSyscallManager::call(uintptr_t function, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
  if (function >= serviceEnd)
  {
    ERROR("PosixSyscallManager: invalid function called: " << Dec << static_cast<int>(function));
    return 0;
  }
  return SyscallManager::instance().syscall(posix, function, p1, p2, p3, p4, p5);
}

uintptr_t PosixSyscallManager::syscall(SyscallState &state)
{
  uintptr_t p1 = state.getSyscallParameter(0);
  uintptr_t p2 = state.getSyscallParameter(1);
  uintptr_t p3 = state.getSyscallParameter(2);
  uintptr_t p4 = state.getSyscallParameter(3);
  uintptr_t p5 = state.getSyscallParameter(4);

  /// \todo This needs to be implemented for posix_recvfrom/posix_sendto
  // uintptr_t p6 = state.getSyscallParameter(5);

  //NOTICE("[" << Processor::information().getCurrentThread()->getParent()->getId() << "] : " << Dec << state.getSyscallNumber() << Hex);

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
    case POSIX_LSEEK:
      return posix_lseek(static_cast<int>(p1), static_cast<int>(p2), static_cast<int>(p3));
    case POSIX_SOCKET:
      return posix_socket(static_cast<int>(p1), static_cast<int>(p2), static_cast<int>(p3));
    case POSIX_CONNECT:
      return posix_connect(static_cast<int>(p1), reinterpret_cast<sockaddr*>(p2), static_cast<size_t>(p3));
    case POSIX_SEND:
      return posix_send(static_cast<int>(p1), reinterpret_cast<void*>(p2), static_cast<size_t>(p3), static_cast<int>(p4));
    case POSIX_RECV:
      return posix_recv(static_cast<int>(p1), reinterpret_cast<void*>(p2), static_cast<size_t>(p3), static_cast<int>(p4));
    case POSIX_BIND:
      return posix_bind(static_cast<int>(p1), reinterpret_cast<sockaddr*>(p2), static_cast<size_t>(p3));
    case POSIX_LISTEN:
      return posix_listen(static_cast<int>(p1), static_cast<int>(p2));
    case POSIX_ACCEPT:
      return posix_accept(static_cast<int>(p1), reinterpret_cast<sockaddr*>(p2), reinterpret_cast<size_t*>(p3));
    case POSIX_RECVFROM:
      return posix_recvfrom(reinterpret_cast<void*>(p1));
    case POSIX_SENDTO:
      return posix_sendto(reinterpret_cast<void*>(p1));
    case POSIX_GETTIMEOFDAY:
      return posix_gettimeofday(reinterpret_cast<struct timeval *>(p1), reinterpret_cast<struct timezone *>(p2));
    case POSIX_DUP:
      return posix_dup(static_cast<int>(p1));
    case POSIX_DUP2:
      return posix_dup2(static_cast<int>(p1), static_cast<int>(p2));
    case POSIX_LSTAT:
      return posix_lstat(reinterpret_cast<char*>(p1), reinterpret_cast<struct stat*>(p2));
    case POSIX_UNLINK:
      return posix_unlink(reinterpret_cast<char*>(p1));
    case POSIX_GETHOSTBYNAME:
      return posix_gethostbyname(reinterpret_cast<const char*>(p1), reinterpret_cast<void*>(p2), static_cast<int>(p3));
    case POSIX_GETHOSTBYADDR:
      return posix_gethostbyaddr(reinterpret_cast<const void*>(p1), static_cast<unsigned long>(p2), static_cast<int>(p3), reinterpret_cast<void*>(p4));
    case POSIX_FCNTL:
      return posix_fcntl(static_cast<int>(p1), static_cast<int>(p2), static_cast<int>(p3), reinterpret_cast<int*>(p4));
    case POSIX_PIPE:
      return posix_pipe(reinterpret_cast<int*>(p1));
    case POSIX_MKDIR:
      return posix_mkdir(reinterpret_cast<const char*>(p1), static_cast<int>(p2));
    case POSIX_GETPWENT:
      return posix_getpwent(reinterpret_cast<passwd*>(p1), static_cast<int>(p2), reinterpret_cast<char *>(p3));
    case POSIX_GETPWNAM:
      return posix_getpwnam(reinterpret_cast<passwd*>(p1), reinterpret_cast<const char*>(p2), reinterpret_cast<char *>(p3));
    case POSIX_GETUID:
      return posix_getuid();
    case POSIX_GETGID:
      return posix_getgid();
    case POSIX_SIGACTION:
      return posix_sigaction(static_cast<int>(p1), reinterpret_cast<const sigaction*>(p2), reinterpret_cast<sigaction*>(p3));
    case POSIX_SIGNAL:
      return posix_signal(static_cast<int>(p1), reinterpret_cast<void*>(p2));
    case POSIX_RAISE:
      return posix_raise(static_cast<int>(p1));
    case POSIX_KILL:
      return posix_kill(static_cast<int>(p1), static_cast<int>(p2));
    case POSIX_SIGPROCMASK:
      return posix_sigprocmask(static_cast<int>(p1), reinterpret_cast<const uint32_t*>(p2), reinterpret_cast<uint32_t*>(p3));
    case POSIX_ALARM:
      return posix_alarm(static_cast<uint32_t>(p1));
    case POSIX_SLEEP:
      return posix_sleep(static_cast<uint32_t>(p1));
    case POSIX_STUBBED:
      WARNING("Using stubbed function '" << reinterpret_cast<const char*>(p1) << "'");
      return 0;
    case PEDIGREE_LOGIN:
      return pedigree_login(static_cast<int>(p1), reinterpret_cast<const char *>(p2));
    case PEDIGREE_SIGRET:
      return pedigree_sigret();
    default: ERROR ("PosixSyscallManager: invalid syscall received: " << Dec << state.getSyscallNumber()); return 0;
  }
}
