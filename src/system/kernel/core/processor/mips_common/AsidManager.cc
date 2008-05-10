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

#include "AsidManager.h"
#include <Log.h>
#include <panic.h>

AsidManager AsidManager::m_Instance;

AsidManager &AsidManager::instance()
{
  return m_Instance;
}

AsidManager::AsidManager()
{
}

AsidManager::~AsidManager()
{
}

AsidManager::Asid AsidManager::obtainAsid()
{
  // Our algorithm is a linear search of multiple levels.
  // Firstly we search for an ASID with a contention of zero.
  // if that's not found, we search for one with <= 1,
  // etc. The reason for the less than is that an asid could be given back
  // while we are searching, and this covers all eventualities.

  for (int i = 0; i < 10; i++)
  {
    for (int j = 0; j < NUM_ASID; j++)
    {
      if (m_Asids[j] <= i)
      {
        // Lock it.
        m_Mutex.acquire();
        // Is it still valid?
        if (m_Asids[j] <= 1)
        {
          // Success! If this ASID is contended, clear the TLB.
          if (m_Asids[j] > 0)
            bulldoze(j);
          m_Asids[j] ++;
          m_Mutex.release();
          return j;
        }
        // No? Then release the lock and keep searching.
        m_Mutex.release();
      }
    }
  }
  // If we got here then we didn't find any asid with less than a contention
  // of 10. This is really really bad. Panic here.
  panic("Too many processes running, not enough ASIDs!");
}

void AsidManager::returnAsid(AsidManager::Asid asid)
{
  // Grab the lock.
  m_Mutex.acquire();
  // Sanity check...
  if (m_Asids[asid] == 0)
    ERROR("returnAsid called on an ASID that hasn't been allocated!");
  m_Asids[asid] --;
  // Release the lock.
  m_Mutex.release();
}

void AsidManager::bulldoze(AsidManager::Asid asid)
{
  // TODO:: implement
}
