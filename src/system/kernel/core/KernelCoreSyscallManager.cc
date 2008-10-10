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

#include <processor/KernelCoreSyscallManager.h>
#include <processor/SyscallManager.h>
#include <processor/Processor.h>
#include <process/Scheduler.h>
#include <Log.h>

KernelCoreSyscallManager KernelCoreSyscallManager::m_Instance;

KernelCoreSyscallManager::KernelCoreSyscallManager()
{
}

KernelCoreSyscallManager::~KernelCoreSyscallManager()
{
}

void KernelCoreSyscallManager::initialise()
{
  SyscallManager::instance().registerSyscallHandler(kernelCore, this);
}

uintptr_t KernelCoreSyscallManager::call(Function_t function, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
  // if (function >= serviceEnd)
  // {
  //   ERROR("KernelCoreSyscallManager: invalid function called: " << Dec << static_cast<int>(function));
  //   return 0;
  // }
  return SyscallManager::instance().syscall(kernelCore, function, p1, p2, p3, p4, p5);
}

uintptr_t KernelCoreSyscallManager::syscall(SyscallState &state)
{
  switch (state.getSyscallNumber())
  {
    case yield: Scheduler::instance().schedule(0, state, reinterpret_cast<Thread*> (state.getSyscallParameter(0))); return 0;
    default: ERROR ("KernelCoreSyscallManager: invalid syscall received: " << Dec << state.getSyscallNumber()); return 0;
  }
}
