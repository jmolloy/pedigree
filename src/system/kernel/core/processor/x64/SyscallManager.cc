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

#include <compiler.h>
#include <LockGuard.h>
#include <processor/Processor.h>
#include "SyscallManager.h"

X64SyscallManager X64SyscallManager::m_Instance;

SyscallManager &SyscallManager::instance()
{
  return X64SyscallManager::instance();
}

bool X64SyscallManager::registerSyscallHandler(Service_t Service, SyscallHandler *pHandler)
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

void X64SyscallManager::syscall(SyscallState &syscallState)
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

uintptr_t X64SyscallManager::syscall(Service_t service, uintptr_t function, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
    uint64_t rax = (static_cast<uint64_t>(service) << 16) | static_cast<uint64_t>(function);
    uint64_t ret;
    asm volatile("mov %6, %%r8; \
                  syscall" : "=a" (ret)
                 : "0" (rax), "b" (p1), "d" (p2), "S" (p3), "D" (p4), "m" (p5)
                 : "rcx", "r11");
    return ret;
}

//
// Functions only usable in the kernel initialisation phase
//

extern "C" void syscall_handler();
void X64SyscallManager::initialiseProcessor()
{
  // Enable SCE (= System Call Extensions)
  // Set IA32_EFER/EFER.SCE
  Processor::writeMachineSpecificRegister(0xC0000080, Processor::readMachineSpecificRegister(0xC0000080) | 0x0000000000000001);

  // Setup SYSCALL/SYSRET
  // Set the IA32_STAR/STAR (CS/SS segment selectors)
  Processor::writeMachineSpecificRegister(0xC0000081, 0x001B000800000000LL);
  // Set the IA32_LSTAR/LSTAR (RIP)
  Processor::writeMachineSpecificRegister(0xC0000082, reinterpret_cast<uint64_t>(syscall_handler));
  // Set the IA32_FMASK/SF_MASK (RFLAGS mask, RFLAGS.IF cleared after syscall)
  Processor::writeMachineSpecificRegister(0xC0000084,  0x0000000000000200LL);
}

X64SyscallManager::X64SyscallManager()
  : m_Lock()
{
  // Initialise the pointers to the handler
  for (size_t i = 0;i < serviceEnd;i++)
    m_pHandler[i] = 0;
}
X64SyscallManager::~X64SyscallManager()
{
}
