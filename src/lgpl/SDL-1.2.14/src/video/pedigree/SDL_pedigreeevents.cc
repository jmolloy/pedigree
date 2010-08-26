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

#include "SDL.h"
#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"

#include "SDL_pedigreevideo.h"
#include "SDL_pedigreeevents_c.h"

#include <syslog.h>

#include <input/Input.h>

// Input handler: receives input notifications and sends them to SDL
void input_handler(Input::InputNotification &note)
{
    if(note.type == Input::Key)
    {
        SDL_keysym sym;
        sym.scancode = 0;
        sym.sym = static_cast<SDLKey>(note.data.key.key);
        sym.mod = KMOD_NONE;
        sym.unicode = 0;
        SDL_PrivateKeyboard(SDL_PRESSED, &sym);
        SDL_PrivateKeyboard(SDL_RELEASED, &sym);
    }
    else if(note.type == Input::Mouse)
    {
        Uint8 state = 0;
        for(int i = 0; i < 8; i++)
            if(note.data.pointy.buttons[i])
                state |= (1 << i);

        SDL_PrivateMouseMotion(state, 1, (Sint16) note.data.pointy.relx, (Sint16) note.data.pointy.rely);
    }
}

void PEDIGREE_InitInput()
{
    Input::installCallback(Input::Key | Input::Mouse, input_handler);
}

void PEDIGREE_DestroyInput()
{
    Input::removeCallback(input_handler);
}

void PEDIGREE_PumpEvents(_THIS)
{
	// Nothing that has to be done here - everything comes in as events
}

void PEDIGREE_InitOSKeymap(_THIS)
{
	/// \todo Need to setup the keymap for stuff like the enter key!
}

