/*
 * Copyright (c) 2010 Matthew Iselin
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
#include <graphics/GraphicsService.h>
#include <processor/types.h>

bool GraphicsService::serve(ServiceFeatures::Type type, void *pData, size_t dataLen)
{
    if(!pData)
        return false;

    // Touch = provide a new display device
    GraphicsProvider *pProvider = reinterpret_cast<GraphicsProvider *>(pData);
    if(type & ServiceFeatures::touch)
    {
        /// \todo Sanity check
        m_Providers.pushBack(pProvider);
        
        m_pCurrentProvider = determineBestProvider();
        
        return true;
    }
    else if(type & ServiceFeatures::probe)
    {
        // Return the current provider if there is one
        /// \todo Sanity check
        if(m_pCurrentProvider)
            memcpy(pData, m_pCurrentProvider, sizeof(GraphicsProvider));
        else
            return false;
        
        return true;
    }

    // Invalid command
    return false;
}

GraphicsService::GraphicsProvider *GraphicsService::determineBestProvider()
{
    GraphicsProvider *pBest = 0;
    uint64_t bestPoints = 0;
    for(List<GraphicsProvider*>::Iterator it = m_Providers.begin();
        it != m_Providers.end();
        it++)
    {
        if(!*it)
            continue;
        
        GraphicsProvider *pProvider = *it;
        
        uint64_t points = 0;
        
        // Hardware acceleration points
        if(pProvider->bHardwareAccel)
            points += (0x10000ULL * 0x10000ULL) * 32ULL; // 16384x16384x32, hard to beat if no hardware accel
        
        // Maximums points (highest resolution in bits)
        points += static_cast<uint64_t>(pProvider->maxWidth) * static_cast<uint64_t>(pProvider->maxHeight) * static_cast<uint64_t>(pProvider->maxDepth);
        
        // Is this the new best?
        bool bNewBest = false;
        if(points > bestPoints)
        {
            bestPoints = points;
            pBest = pProvider;
            
            bNewBest = true;
        }
        
        String name;
        pProvider->pDisplay->getName(name);
        DEBUG_LOG("GraphicsService: provider with display name '" << name << "' got " << points << " points" << (bNewBest ? " [new best choice]" : ""));
    }
    return pBest;
}

