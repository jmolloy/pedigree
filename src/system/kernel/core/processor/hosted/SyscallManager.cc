/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include <compiler.h>
#include <LockGuard.h>
#include <processor/Processor.h>
#include "SyscallManager.h"

HostedSyscallManager HostedSyscallManager::m_Instance;

SyscallManager &SyscallManager::instance()
{
  return HostedSyscallManager::instance();
}

bool HostedSyscallManager::registerSyscallHandler(Service_t Service, SyscallHandler *pHandler)
{
  // Lock the class until the end of the function
  LockGuard<Spinlock> lock(m_Lock);

  if (UNLIKELY(Service >= serviceEnd))
    return false;
  if (UNLIKELY(pHandler != 0 && m_pHandler[Service] != 0))
    return false;
  if (UNLIKELY(pHandler == 0 && m_pHandler[Service] == 0))
    return false;

  m_pHandler[Service] = pHandler;
  return true;
}

void HostedSyscallManager::syscall(SyscallState &syscallState)
{
  SyscallHandler *pHandler;
  size_t serviceNumber = syscallState.getSyscallService();

  if (UNLIKELY(serviceNumber >= serviceEnd))
  {
    // TODO: We should return an error here
    return;
  }

  // Get the syscall handler
  {
    LockGuard<Spinlock> lock(m_Instance.m_Lock);
    pHandler = m_Instance.m_pHandler[serviceNumber];
  }
  
  if (LIKELY(pHandler != 0))
  {
    syscallState.setSyscallReturnValue(pHandler->syscall(syscallState));
    syscallState.setSyscallErrno(Processor::information().getCurrentThread()->getErrno());

    if (Processor::information().getCurrentThread()->getUnwindState() == Thread::Exit)
    {
      NOTICE("Unwind state exit, in interrupt handler");
      Processor::information().getCurrentThread()->getParent()->getSubsystem()->exit(0);
    }
  }
}

uintptr_t HostedSyscallManager::syscall(Service_t service, uintptr_t function, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
  return 0;
}

//
// Functions only usable in the kernel initialisation phase
//

void HostedSyscallManager::initialiseProcessor()
{
}

HostedSyscallManager::HostedSyscallManager()
  : m_Lock()
{
  // Initialise the pointers to the handler
  for (size_t i = 0;i < serviceEnd;i++)
    m_pHandler[i] = 0;
}

HostedSyscallManager::~HostedSyscallManager()
{
}
