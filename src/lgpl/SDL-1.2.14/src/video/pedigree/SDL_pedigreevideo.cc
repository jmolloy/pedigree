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

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_pedigreevideo.h"
#include "SDL_pedigreeevents_c.h"
#include "SDL_pedigreemouse_c.h"

#include <graphics/Graphics.h>

#include <Widget.h>

#include <syslog.h>

#include <cairo/cairo.h>

#include <string>

#define PEDIGREEVID_DRIVER_NAME "pedigree"

extern "C"
{

/* Initialization/Query functions */
static int PEDIGREE_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **PEDIGREE_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *PEDIGREE_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int PEDIGREE_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void PEDIGREE_VideoQuit(_THIS);

/* Hardware surface functions */
static int PEDIGREE_AllocHWSurface(_THIS, SDL_Surface *surface);
static int PEDIGREE_LockHWSurface(_THIS, SDL_Surface *surface);
static void PEDIGREE_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void PEDIGREE_FreeHWSurface(_THIS, SDL_Surface *surface);

/* etc. */
static void PEDIGREE_UpdateRects(_THIS, int numrects, SDL_Rect *rects);
static void PEDIGREE_SetWMCaption(_THIS, const char *title, const char *icon);

};

/** PEDIGREE SDL Widget type. */
PEDIGREE_SDLWidget *g_Widget = 0;

bool PEDIGREE_SDLwidgetCallback(WidgetMessages message, size_t msgSize, const void *msgData);

/* PEDIGREE driver bootstrap functions */

static int PEDIGREE_Available(void)
{
    return 1;
}

static void PEDIGREE_DeleteDevice(SDL_VideoDevice *device)
{
    PEDIGREE_DestroyInput();
    
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_VideoDevice *PEDIGREE_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				SDL_malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			SDL_free(device);
		}
		return(0);
	}
	SDL_memset(device->hidden, 0, (sizeof *device->hidden));

	/* Set the function pointers */
	device->VideoInit = PEDIGREE_VideoInit;
	device->ListModes = PEDIGREE_ListModes;
	device->SetVideoMode = PEDIGREE_SetVideoMode;
	device->CreateYUVOverlay = NULL;
	device->SetColors = PEDIGREE_SetColors;
	device->UpdateRects = PEDIGREE_UpdateRects;
	device->VideoQuit = PEDIGREE_VideoQuit;
	device->AllocHWSurface = PEDIGREE_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = PEDIGREE_LockHWSurface;
	device->UnlockHWSurface = PEDIGREE_UnlockHWSurface;
	device->FlipHWSurface = NULL;
	device->FreeHWSurface = PEDIGREE_FreeHWSurface;
	device->SetCaption = PEDIGREE_SetWMCaption;
	device->SetIcon = NULL;
	device->IconifyWindow = NULL;
	device->GrabInput = NULL;
	device->GetWMInfo = NULL;
	device->InitOSKeymap = PEDIGREE_InitOSKeymap;
	device->PumpEvents = PEDIGREE_PumpEvents;

	device->free = PEDIGREE_DeleteDevice;

	return device;
}

VideoBootStrap PEDIGREE_bootstrap = {
	PEDIGREEVID_DRIVER_NAME, "SDL pedigree video driver",
	PEDIGREE_Available, PEDIGREE_CreateDevice
};


int PEDIGREE_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	// Default to 16 bpp
	vformat->BitsPerPixel = 32;
	vformat->BytesPerPixel = 4;

        char endpoint[256];
        sprintf(endpoint, "sdl.%d", getpid());

        // Create widget.
        if(!g_Widget)
        {
            PedigreeGraphics::Rect rt;
            g_Widget = new PEDIGREE_SDLWidget();
            if(!g_Widget->construct(endpoint, "SDL", PEDIGREE_SDLwidgetCallback, rt))
            {
                fprintf(stderr, "SDL: couldn't construct widget\n");
                delete g_Widget;
                return -1;
            }
            _this->hidden->widget = g_Widget;
        }

    // Bring up the input callbacks
    PEDIGREE_InitInput();

	/* We're done! */
	return(0);
}

SDL_Rect **PEDIGREE_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
    /// \todo Write?
    /// \note If we bring up a window in VideoInit, we can possibly get its resolution
    ///       and list it as the only available mode in this list...
   	return (SDL_Rect **) -1;
}

SDL_Surface *PEDIGREE_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	if ( _this->hidden->buffer ) {
		SDL_free( _this->hidden->buffer );
	}

    syslog(LOG_INFO, "SetVideoMode(%d, %d, %d)", width, height, bpp);

	_this->hidden->buffer = SDL_malloc(width * height * 4);
	if ( ! _this->hidden->buffer ) {
		SDL_SetError("Couldn't allocate buffer for requested mode");
		return(NULL);
	}

	SDL_memset(_this->hidden->buffer, 0, width * height * 4);

	/* Set up the new mode framebuffer */
	current->flags = flags;
        _this->hidden->w = current->w = width;
	_this->hidden->h = current->h = height;
	current->pitch = current->w * 4;
	current->pixels = _this->hidden->buffer;

	/* We're done */
	return(current);
}

/* We don't actually allow hardware surfaces other than the main one */
static int PEDIGREE_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return(-1);
}
static void PEDIGREE_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int PEDIGREE_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}

static void PEDIGREE_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static void PEDIGREE_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
    if(!(_this->hidden->widget && _this->hidden->buffer))
        return;

    // Batch into one Rect so we don't have hundreds of little redraws.
    PedigreeGraphics::Rect dirty;
    size_t minX = ~0, minY = ~0, maxX = 0, maxY = 0;
    for(int i = 0; i < numrects; i++)
    {
        size_t x1 = rects[i].x;
        size_t x2 = x1 + rects[i].w;

        size_t y1 = rects[i].y;
        size_t y2 = rects[i].h;

        minX = std::min(minX, x1);
        minY = std::min(minY, y1);

        maxX = std::max(maxX, x2);
        maxY = std::max(maxY, y2);
    }

    size_t redrawWidth = maxX - minX;
    size_t redrawHeight = maxY - minY;

    // Clip.
    if(minX > _this->hidden->widget->getWidth())
        return;
    if(minY > _this->hidden->widget->getHeight())
        return;

    if((minX + redrawWidth) > _this->hidden->widget->getWidth())
        redrawWidth = _this->hidden->widget->getWidth() - minX;
    if((minY + redrawHeight) > _this->hidden->widget->getHeight())
        redrawHeight = _this->hidden->widget->getHeight() - minY;

    // Grab the buffer to draw to.
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, _this->hidden->widget->getWidth());
    cairo_surface_t *destSurface = cairo_image_surface_create_for_data(
            (uint8_t *) _this->hidden->widget->getRawFramebuffer(),
            CAIRO_FORMAT_ARGB32,
            _this->hidden->widget->getWidth(),
            _this->hidden->widget->getHeight(),
            stride);
    cairo_t *cr = cairo_create(destSurface);

    // Grab the source buffer.
    stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, _this->hidden->w);
    cairo_surface_t *sourceSurface = cairo_image_surface_create_for_data(
            (uint8_t *) _this->hidden->buffer,
            CAIRO_FORMAT_RGB24,
            _this->hidden->w,
            _this->hidden->h,
            stride);

    // Set the clip mask to the bounds of the target.
    cairo_rectangle(cr, minX, minY, redrawWidth, redrawHeight);
    cairo_clip(cr);

    // Prepare.
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, sourceSurface, 0.0, 0.0);
    cairo_paint_with_alpha(cr, 1.0);

    // Flush.
    cairo_surface_flush(destSurface);

    // Destroy surfaces.
    cairo_surface_destroy(destSurface);
    cairo_surface_destroy(sourceSurface);
    cairo_destroy(cr);

    // Perform the redraw.
    dirty.update(minX, minY, redrawWidth, redrawHeight);
    _this->hidden->widget->redraw(dirty);
}

int PEDIGREE_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
    /* No palette support. */
    return 1;
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void PEDIGREE_VideoQuit(_THIS)
{
	if (_this->screen->pixels != NULL)
	{
		SDL_free(_this->screen->pixels);
		_this->screen->pixels = NULL;
	}

    _this->hidden->widget->destroy();
    delete _this->hidden->widget;
    _this->hidden->widget = 0;
}

static void PEDIGREE_SetWMCaption(_THIS, const char *title, const char *icon)
{
    if(!_this->hidden->widget)
        return;

    _this->hidden->widget->setTitle(std::string(title));
}
