/*
 * Copyright (c) 2010 Matthew Iselin, Heights College
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
 
#include "Filter.h"
#include <Log.h>

NetworkFilter NetworkFilter::m_Instance;

NetworkFilter::NetworkFilter() : m_Callbacks()
{
}

NetworkFilter::~NetworkFilter()
{
}

bool NetworkFilter::filter(size_t level, uintptr_t packet, size_t sz)
{
    // Check for a valid level
    if(level <= 4 && level > 0)
    {
        // Grab the callback list
        typedef List<void*>::Iterator callbackIterator;
        List<void*> *list = m_Callbacks.lookup(level);
        if(list)
        {
            // Iterate, call each callback until one returns false
            for(callbackIterator it = list->begin(); it != list->end(); ++it)
            {
                bool (*callback)(uintptr_t, size_t) = reinterpret_cast<bool (*)(uintptr_t, size_t)>(*it);
                bool result = callback(packet, sz);
                if(!result)
                    return result; // Short-circuit. This way we avoid executing
                                   // extra filters if one says to drop.
            }
        }
    }
    
    // Default response: allow packet
    return true;
}

size_t NetworkFilter::installCallback(size_t level, bool (*callback)(uintptr_t, size_t))
{
    /// \todo UnlikelyLock here

    // Check for a valid level
    if(level <= 4 && level > 0)
    {
        // Grab the callback list
        List<void*> *list = m_Callbacks.lookup(level);
        
        // If it already exists, add the callback
        if(list)
        {
            // We return the index into the list of this callback
            size_t index = list->count();
            list->pushBack(reinterpret_cast<void*>(callback));
            return index;
        }
        // Otherwise, allocate
        else
        {
            list = new List<void*>;
            if(!list)
            {
                ERROR("Ran out of memory creating list for level " << Dec << level << Hex << " callbacks!");
                return (size_t) -1;
            }
            
            list->pushBack(reinterpret_cast<void*>(callback));
            m_Callbacks.insert(level, list);
            
            // First item
            return 0;
        }
    }
    
    // Invalid input
    return (size_t) -1;
}

void NetworkFilter::removeCallback(size_t level, size_t id)
{
    /// \todo Implement me!
}
