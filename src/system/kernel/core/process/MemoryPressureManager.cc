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


#include <process/MemoryPressureManager.h>
#include <Log.h>

MemoryPressureManager MemoryPressureManager::m_Instance;

bool MemoryPressureManager::compact()
{
    for(size_t i = 0; i < MAX_MEMPRESSURE_PRIORITY; ++i)
    {
        for (List<MemoryPressureHandler *>::Iterator it = m_Handlers[i].begin();
            it != m_Handlers[i].end();
            ++it)
        {
            NOTICE("Compact: " << (*it)->getMemoryPressureDescription());
            if((*it)->compact())
            {
                NOTICE("  -> pages released!");
                return true;
            }
            NOTICE("  -> no pages released.");
        }
    }

    return false;
}

void MemoryPressureManager::registerHandler(size_t prio, MemoryPressureHandler *pHandler)
{
    if(prio >= MAX_MEMPRESSURE_PRIORITY)
        prio = MAX_MEMPRESSURE_PRIORITY - 1;

    m_Handlers[prio].pushBack(pHandler);
}

void MemoryPressureManager::removeHandler(MemoryPressureHandler *pHandler)
{
    for(size_t i = 0; i < MAX_MEMPRESSURE_PRIORITY; ++i)
    {
        for (List<MemoryPressureHandler *>::Iterator it = m_Handlers[i].begin();
            it != m_Handlers[i].end();
            )
        {
            if ((*it) == pHandler)
            {
                it = m_Handlers[i].erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}
