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

#include "InterruptManager.h"
#include <machine/Machine.h>
#include <machine/types.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <Debugger.h>
#include <Log.h>


PPC32InterruptManager PPC32InterruptManager::m_Instance;

SyscallManager &SyscallManager::instance()
{
  return PPC32InterruptManager::instance();
}
InterruptManager &InterruptManager::instance()
{
  return PPC32InterruptManager::instance();
}

bool PPC32InterruptManager::registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler)
{
  return true;
}

#ifdef DEBUGGER

  bool PPC32InterruptManager::registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler)
  {
    return true;
  }
  size_t PPC32InterruptManager::getBreakpointInterruptNumber()
  {
    return 3;
  }
  size_t PPC32InterruptManager::getDebugInterruptNumber()
  {
    return 1;
  }

#endif

bool PPC32InterruptManager::registerSyscallHandler(Service_t Service, SyscallHandler *handler)
{
  return true;
}

void PPC32InterruptManager::initialiseProcessor()
{
}

void PPC32InterruptManager::interrupt(InterruptState &interruptState)
{
}

PPC32InterruptManager::PPC32InterruptManager()
{
}
PPC32InterruptManager::~PPC32InterruptManager()
{
}
