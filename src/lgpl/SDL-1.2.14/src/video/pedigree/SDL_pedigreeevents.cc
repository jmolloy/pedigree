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
#include <Widget.h>

extern PEDIGREE_SDLWidget *g_Widget;

static SDLKey keymap[256];
static SDLMod modstate = KMOD_NONE;

// Receives a HID scancode, translates it, and submits to SDL.
static void handle_hid_scancode(uint16_t scancode, bool bUp)
{
    SDLKey trans = keymap[scancode];

    SDL_keysym sym;
    sym.scancode = scancode;
    sym.unicode = 0;
    sym.mod = KMOD_NONE;
    sym.sym = trans;

    int mods = static_cast<int>(modstate);

    if(bUp)
    {
        if(trans == SDLK_LALT)
            mods &= ~KMOD_LALT;
        if(trans == SDLK_RALT)
            mods &= ~KMOD_RALT;
        if(trans == SDLK_LSHIFT)
            mods &= ~KMOD_LSHIFT;
        if(trans == SDLK_RSHIFT)
            mods &= ~KMOD_RSHIFT;
        if(trans == SDLK_LCTRL)
            mods &= ~KMOD_LCTRL;
        if(trans == SDLK_RCTRL)
            mods &= ~KMOD_RCTRL;

        sym.mod = static_cast<SDLMod>(mods);

        SDL_PrivateKeyboard(SDL_RELEASED, &sym);
    }
    else
    {
        if(trans == SDLK_LALT)
            mods |= KMOD_LALT;
        if(trans == SDLK_RALT)
            mods |= KMOD_RALT;
        if(trans == SDLK_LSHIFT)
            mods |= KMOD_LSHIFT;
        if(trans == SDLK_RSHIFT)
            mods |= KMOD_RSHIFT;
        if(trans == SDLK_LCTRL)
            mods |= KMOD_LCTRL;
        if(trans == SDLK_RCTRL)
            mods |= KMOD_RCTRL;

        sym.mod = static_cast<SDLMod>(mods);

        SDL_PrivateKeyboard(SDL_PRESSED, &sym);
    }

    modstate = static_cast<SDLMod>(mods);
}

// Input handler: receives input notifications and sends them to SDL
static void input_handler(Input::InputNotification &note)
{
    if(note.type == Input::Mouse)
    {
        for(int i = 0; i < 8; i++)
        {
            if(note.data.pointy.buttons[i] && !(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(i + 1)))
                SDL_PrivateMouseButton(SDL_PRESSED, i + 1, 0, 0);
            if(!note.data.pointy.buttons[i] && (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(i + 1)))
                SDL_PrivateMouseButton(SDL_RELEASED, i + 1, 0, 0);
        }

        SDL_PrivateMouseMotion(0, 1, (Sint16) note.data.pointy.relx, (Sint16) note.data.pointy.rely);
    }
}

void PEDIGREE_InitInput()
{
    /// \todo Mouse support in the window manager.
    Input::installCallback(Input::Mouse, input_handler);
}

void PEDIGREE_DestroyInput()
{
    Input::removeCallback(input_handler);
}

void PEDIGREE_PumpEvents(_THIS)
{
	// Nothing that has to be done here - everything comes in as async events
    Widget::checkForEvents(true);
}

bool PEDIGREE_SDLwidgetCallback(WidgetMessages message, size_t msgSize, void *msgData)
{
    switch(message)
    {
        case Reposition:
            {
                PedigreeGraphics::Rect *rt = reinterpret_cast<PedigreeGraphics::Rect *>(msgData);
                g_Widget->reposition(*rt);
            }
            break;
        case RawKeyUp:
        case RawKeyDown:
            {
                uint16_t scancode = *reinterpret_cast<uint16_t*>(msgData);
                handle_hid_scancode(scancode, message == RawKeyUp);
            }
            break;
        case Terminate:
            SDL_PrivateQuit();
            break;
        default:
            break;
    }
    return true;
}

void PEDIGREE_InitOSKeymap(_THIS)
{
    size_t i;

    /* Initialize the DirectFB key translation table */
    for(i = 0; i < SDL_arraysize(keymap); ++i)
        keymap[i] = SDLK_UNKNOWN;

    for(i = 0; i <= (SDLK_z - SDLK_a); i++)
        keymap[0x04 + i]    = static_cast<SDLKey>(SDLK_a + i);

    for(i = 0; i <= (SDLK_9 - SDLK_1); i++)
        keymap[0x1E + i]    = static_cast<SDLKey>(SDLK_1 + i);
    keymap[0x27] = SDLK_0;

    keymap[0x28]            = SDLK_RETURN;
    keymap[0x29]            = SDLK_ESCAPE;
    keymap[0x2A]            = SDLK_BACKSPACE;
    keymap[0x2B]            = SDLK_TAB;
    keymap[0x2C]            = SDLK_SPACE;
    keymap[0x2D]            = SDLK_MINUS;
    keymap[0x2E]            = SDLK_PLUS;
    keymap[0x2F]            = SDLK_LEFTBRACKET;
    keymap[0x30]            = SDLK_RIGHTBRACKET;
    keymap[0x31]            = SDLK_BACKSLASH;
    keymap[0x32]            = SDLK_UNKNOWN; // INT 2?
    keymap[0x33]            = SDLK_SEMICOLON;
    keymap[0x34]            = SDLK_ASTERISK;
    keymap[0x35]            = SDLK_BACKQUOTE;
    keymap[0x36]            = SDLK_COMMA;
    keymap[0x37]            = SDLK_PERIOD;
    keymap[0x38]            = SDLK_SLASH;
    keymap[0x39]            = SDLK_UNKNOWN; // Caps Lock

    for(i = 0; i <= (SDLK_F12 - SDLK_F1); i++)
        keymap[0x3A + i]    = static_cast<SDLKey>(SDLK_F1 + i);

    keymap[0x46]            = SDLK_UNKNOWN; // Print screen
    keymap[0x47]            = SDLK_UNKNOWN; // Scroll lock
    keymap[0x48]            = SDLK_UNKNOWN; // Pause/Break
    keymap[0x49]            = SDLK_INSERT;
    keymap[0x4A]            = SDLK_HOME;
    keymap[0x4B]            = SDLK_PAGEUP;
    keymap[0x4C]            = SDLK_DELETE;
    keymap[0x4D]            = SDLK_END;
    keymap[0x4E]            = SDLK_PAGEDOWN;
    keymap[0x4F]            = SDLK_RIGHT;
    keymap[0x50]            = SDLK_LEFT;
    keymap[0x51]            = SDLK_DOWN;
    keymap[0x52]            = SDLK_UP;
    keymap[0x53]            = SDLK_UNKNOWN; // Numlock
    keymap[0x54]            = SDLK_KP_DIVIDE;
    keymap[0x55]            = SDLK_KP_MULTIPLY;
    keymap[0x56]            = SDLK_KP_MINUS;
    keymap[0x57]            = SDLK_KP_PLUS;
    keymap[0x58]            = SDLK_KP_ENTER;

    keymap[0x59]            = SDLK_KP1;
    keymap[0x5A]            = SDLK_KP2;
    keymap[0x5B]            = SDLK_KP3;
    keymap[0x5C]            = SDLK_KP4;
    keymap[0x5E]            = SDLK_KP6;
    keymap[0x5F]            = SDLK_KP7;
    keymap[0x60]            = SDLK_KP8;
    keymap[0x61]            = SDLK_KP9;
    keymap[0x62]            = SDLK_KP0;

    keymap[0x63]            = SDLK_KP_PERIOD;
    keymap[0x64]            = SDLK_UNKNOWN; // INT 1?
    keymap[0x65]            = SDLK_LSUPER; // WinMenu, turn into super
    // 0x68 -> 0x73 = F13 to F24
    keymap[0x75]            = SDLK_HELP;
    keymap[0x7A]            = SDLK_UNDO;

    keymap[0x97]            = SDLK_KP5;

    keymap[0xE0]            = SDLK_LCTRL;
    keymap[0xE1]            = SDLK_LSHIFT;
    keymap[0xE2]            = SDLK_LALT;
    keymap[0xE3]            = SDLK_LSUPER;
    keymap[0xE4]            = SDLK_RCTRL;
    keymap[0xE5]            = SDLK_RSHIFT;
    keymap[0xE6]            = SDLK_RALT;
    keymap[0xE7]            = SDLK_RSUPER;
}

