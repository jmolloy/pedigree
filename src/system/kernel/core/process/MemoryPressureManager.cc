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

MemoryPressureManager MemoryPressureManager::m_Instance;

bool MemoryPressureManager::compact()
{
    for (List<MemoryPressureHandler *>::Iterator it = m_Handlers.begin();
        it != m_Handlers.end();
        ++it)
    {
        if((*it)->compact())
        {
            return true;
        }
    }

    return false;
}

void MemoryPressureManager::registerHandler(MemoryPressureHandler *pHandler)
{
    m_Handlers.pushBack(pHandler);
}

void MemoryPressureManager::removeHandler(MemoryPressureHandler *pHandler)
{
    for (List<MemoryPressureHandler *>::Iterator it = m_Handlers.begin();
        it != m_Handlers.end();
        )
    {
        if ((*it) == pHandler)
        {
            it = m_Handlers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
