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
#ifndef _GRAPHICS_SERVICE_H
#define _GRAPHICS_SERVICE_H

#include <processor/types.h>
#include <ServiceManager.h>

#include <machine/Framebuffer.h>
#include <machine/Display.h>

class GraphicsService : public Service
{
    public:
        GraphicsService() : m_Providers(), m_pCurrentProvider(0) {};
        virtual ~GraphicsService() {};
        
        struct GraphicsProvider
        {
            Display *pDisplay;
            
            /* Some form of hardware caps here... */
            bool bHardwareAccel;
            
            Framebuffer *pFramebuffer;
            
            size_t maxWidth;
            size_t maxHeight;
            size_t maxDepth;
            
            /// Set to true if this display can drop back to a text-based mode
            /// with x86's int 10h thing. If this is false, the driver should
            /// handle "mode zero" as a "disable the video device" mode.
            bool bTextModes;
        };

        /** serve: Interface through which clients interact with the Service */
        bool serve(ServiceFeatures::Type type, void *pData, size_t dataLen);
    
    private:
        GraphicsProvider *determineBestProvider();
    
        List<GraphicsProvider*> m_Providers;
        
        GraphicsProvider *m_pCurrentProvider;
};

#endif

