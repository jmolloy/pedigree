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
#include <Log.h>
#include <Debugger.h>
#include "IrqManager.h"
#include <LockGuard.h>

using namespace __pedigree_hosted;

#include <signal.h>

// TODO: Needs locking

HostedIrqManager HostedIrqManager::m_Instance;

void HostedIrqManager::tick()
{
}

bool HostedIrqManager::control(uint8_t irq, ControlCode code, size_t argument)
{
  return true;
}

irq_id_t HostedIrqManager::registerIsaIrqHandler(uint8_t irq, IrqHandler *handler, bool bEdge)
{
  if (UNLIKELY(irq >= 16))
    return 0;

  // Save the IrqHandler
  m_Handler[irq].pushBack(handler);

  return irq + SIGUSR1;
}

irq_id_t HostedIrqManager::registerPciIrqHandler(IrqHandler *handler, Device *pDevice)
{
  if (UNLIKELY(!pDevice))
    return 0;
  irq_id_t irq = pDevice->getInterruptNumber();
  if (UNLIKELY(irq >= 16))
    return 0;

  // Save the IrqHandler
  m_Handler[irq].pushBack(handler);

  return irq + SIGUSR1;
}

void HostedIrqManager::acknowledgeIrq(irq_id_t Id)
{
}

void HostedIrqManager::unregisterHandler(irq_id_t Id, IrqHandler *handler)
{
  size_t irq = (Id - SIGUSR1);

  // Remove the handler
  for(List<IrqHandler*>::Iterator it = m_Handler[irq].begin();
      it != m_Handler[irq].end();
      it++)
  {
    if(*it == handler)
    {
        m_Handler[irq].erase(it);
        return;
    }
  }
}

bool HostedIrqManager::initialise()
{
  // Register the interrupts
  InterruptManager &IntManager = InterruptManager::instance();
  for (size_t i = 0; i < 2; i++)
    if (IntManager.registerInterruptHandler(SIGUSR1 + i, this) == false)
      return false;

  return true;
}

HostedIrqManager::HostedIrqManager() : m_Lock(false)
{
  for(size_t i = 0; i < 2; i++)
  {
    m_Handler[i].clear();
  }
}

void HostedIrqManager::interrupt(size_t interruptNumber, InterruptState &state)
{
  size_t irq = (interruptNumber - SIGUSR1);

  // Call the irq handler, if any
  if (LIKELY(m_Handler[irq].count() != 0))
  {
    for(List<IrqHandler*>::Iterator it = m_Handler[irq].begin();
        it != m_Handler[irq].end();
        it++)
    {
        (*it)->irq(irq, state);
    }
  }
  else
  {
    NOTICE("HostedIrqManager: unhandled irq #" << irq << " occurred");
  }
}
