/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#include <processor/processor.h>
#include "syscall.h"

X64SyscallManager X64SyscallManager::m_Instance;

SyscallManager &SyscallManager::instance()
{
  return X64SyscallManager::instance();
}

bool X64SyscallManager::registerSyscallHandler(Service_t Service, SyscallHandler *handler)
{
  // TODO: Needs locking

  if (UNLIKELY(Service >= SyscallManager::serviceEnd))
    return false;
  if (UNLIKELY(handler != 0 && m_Handler[Service] != 0))
    return false;
  if (UNLIKELY(handler == 0 && m_Handler[Service] == 0))
    return false;

  m_Handler[Service] = handler;
  return true;
}

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

void X64SyscallManager::syscall(SyscallState &syscallState)
{
  // TODO: Needs locking

  size_t serviceNumber = syscallState.getSyscallService();
  if (LIKELY(serviceNumber < serviceEnd && m_Instance.m_Handler[serviceNumber] != 0))
    m_Instance.m_Handler[serviceNumber]->syscall(syscallState);
}

X64SyscallManager::X64SyscallManager()
{
  // Initialise the pointers to the handler
  for (size_t i = 0;i < SyscallManager::serviceEnd;i++)
    m_Handler[i] = 0;
}
X64SyscallManager::~X64SyscallManager()
{
}
