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

/* Dummy SDL video driver implementation; this is just enough to make an
 *  SDL-based application THINK it's got a working video driver, for
 *  applications that call SDL_Init(SDL_INIT_VIDEO) when they don't need it,
 *  and also for use as a collection of stubs when porting SDL to a new
 *  platform for which you haven't yet written a valid video driver.
 *
 * This is also a great way to determine bottlenecks: if you think that SDL
 *  is a performance problem for a given platform, enable this driver, and
 *  then see if your application runs faster without video overhead.
 *
 * Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "PEDIGREE" by Sam Lantinga.
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_nullvideo.h"
#include "SDL_nullevents_c.h"
#include "SDL_nullmouse_c.h"

#include <graphics/Graphics.h>

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

};

/* PEDIGREE driver bootstrap functions */

static int PEDIGREE_Available(void)
{
    return 1;

	const char *envr = SDL_getenv("SDL_VIDEODRIVER");
	if ((envr) && (SDL_strcmp(envr, PEDIGREEVID_DRIVER_NAME) == 0)) {
		return(1);
	}

	return(0);
}

static void PEDIGREE_DeleteDevice(SDL_VideoDevice *device)
{
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
	device->SetCaption = NULL;
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
	/* fprintf(stderr, "WARNING: You are using the SDL pedigree video driver!\n"); */

	/* Determine the screen depth (use default 8-bit depth) */
	/* we change this during the SDL_SetVideoMode implementation... */
	vformat->BitsPerPixel = 24;
	vformat->BytesPerPixel = 3;

	/* We're done! */
	return(0);
}

SDL_Rect **PEDIGREE_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
   	 return (SDL_Rect **) -1;
}

SDL_Surface *PEDIGREE_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	if ( _this->hidden->buffer ) {
		SDL_free( _this->hidden->buffer );
	}

    PedigreeGraphics::Framebuffer *pFramebuffer = new PedigreeGraphics::Framebuffer();
    if(!pFramebuffer->getRawBuffer())
    {
		_this->hidden->buffer = NULL;
		SDL_SetError("Couldn't get a framebuffer to use");
		return(NULL);
    }
    
    _this->hidden->provider = (void*) pFramebuffer;

	_this->hidden->buffer = SDL_malloc(width * height * (bpp / 8));
	if ( ! _this->hidden->buffer ) {
		SDL_SetError("Couldn't allocate buffer for requested mode");
		return(NULL);
	}

 	/* printf("Setting mode %dx%d\n", width, height); */

	SDL_memset(_this->hidden->buffer, 0, width * height * (bpp / 8));

	/* Allocate the new pixel format for the screen */
	if ( ! SDL_ReallocFormat(current, bpp, 0, 0, 0, 0) ) {
		SDL_free(_this->hidden->buffer);
		_this->hidden->buffer = NULL;
		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		return(NULL);
	}

	/* Set up the new mode framebuffer */
	current->flags = flags & SDL_FULLSCREEN;
	_this->hidden->w = current->w = width;
	_this->hidden->h = current->h = height;
	current->pitch = current->w * (bpp / 8);
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
    if(!_this->hidden->provider)
        return;

    PedigreeGraphics::Framebuffer *pFramebuffer = reinterpret_cast<PedigreeGraphics::Framebuffer*>(_this->hidden->provider);
    
    PedigreeGraphics::PixelFormat format;
    if(_this->screen->format->BitsPerPixel == 16)
        format = PedigreeGraphics::Bits16_Rgb555;
    else if(_this->screen->format->BitsPerPixel == 8)
        format = PedigreeGraphics::Bits8_Rgb332;
    else
        format = PedigreeGraphics::Bits24_Rgb;

    pFramebuffer->draw(_this->hidden->buffer, 0, 0, 0, 0, _this->hidden->w, _this->hidden->h, format);

    for(int i = 0; i < numrects; i++)
    {
        pFramebuffer->redraw(rects[i].x, rects[i].y, rects[i].w, rects[i].h);
    }
}

int PEDIGREE_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	/* do nothing of note. */
	return(1);
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
}
