/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifndef _SDL_pedigreevideo_h
#define _SDL_pedigreevideo_h

#include "../SDL_sysvideo.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *_this

#ifdef __cplusplus
#include <Widget.h>

class PEDIGREE_SDLWidget : public Widget
{
    public:
        PEDIGREE_SDLWidget() : Widget()
        {}

        virtual ~PEDIGREE_SDLWidget()
        {}

        void reposition(const PedigreeGraphics::Rect newrt)
        {
            m_nWidth = newrt.getW();
            m_nHeight = newrt.getH();
        }

        void *getBackbuffer()
        {
            return getRawFramebuffer();
        }

        virtual bool render(PedigreeGraphics::Rect &rt, PedigreeGraphics::Rect &dirty)
        {
            // SDL has its own main loop that will render instead.
            return true;
        }

        size_t getWidth() const
        {
            return m_nWidth;
        }

        size_t getHeight() const
        {
            return m_nHeight;
        }
    private:
        size_t m_nWidth, m_nHeight;
};
#else
struct PEDIGREE_SDLWidget {};
#endif

/* Private display data */

struct SDL_PrivateVideoData {
    int w, h;
    void *buffer;
#ifndef __cplusplus
    struct
#endif
    PEDIGREE_SDLWidget *widget;
};

#endif /* _SDL_nullvideo_h */

