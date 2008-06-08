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

#include "VsidManager.h"

VsidManager VsidManager::m_Instance;

VsidManager &VsidManager::instance()
{
  return m_Instance;
}

VsidManager::VsidManager() :
  m_HighWaterMark(1), m_pStack(0), m_Mutex()
{
}

VsidManager::~VsidManager()
{
}

VsidManager::Vsid VsidManager::obtainVsid()
{
  /// \todo Locking

  // Is the stack NULL?
  if (m_pStack == 0)
    // Return the high water mark and increment.
    return m_HighWaterMark++;

  // Pop the stack.
  VsidStack *pPopped = m_pStack;
  m_pStack = m_pStack->next;

  Vsid vsid = pPopped->vsid;
  delete pPopped;

  return vsid;
}

void VsidManager::returnVsid(VsidManager::Vsid vsid)
{
  /// \todo Locking
  /// \todo decrement high water mark if possible
  
  VsidStack *pPushed = new VsidStack;
  pPushed->vsid = vsid;
  pPushed->next = m_pStack;
  m_pStack = pPushed;
}
