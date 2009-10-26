/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

#include <processor/SyscallManager.h>
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <process/Scheduler.h>
#include <machine/Machine.h>
#include <machine/Keyboard.h>
#include <machine/InputManager.h>
#include <console/Console.h>
#include <Log.h>
#include <Module.h>
#include <config/Config.h>

#include "TuiSyscallManager.h"
#include "Console.h"
#include "syscallNumbers.h"

UserConsole *g_Consoles[256];
UserConsole *g_UserConsole = 0;
size_t g_UserConsoleId = 0;
TuiSyscallManager g_TuiSyscallManager;

extern Display *g_pDisplay;
extern Display::ScreenMode g_ScreenMode;

/// \todo Key presses should go to the TUI immediately, rather than going onto
///       the request queue and waiting for it to complete a write to the
///       screen. At the moment things like CTRL-C are not handled until after
///       a write is complete, which is usually alright, but can cause CTRL-C
///       to appear to simply not work at all.
void callback(uint64_t key)
{
    if (!g_UserConsole)
    {
        WARNING("Key called with no console");
        return;
    }

    // Ensure this is sent with highest priority (0).
    g_UserConsole->addAsyncRequest(0, TUI_CHAR_RECV, g_UserConsoleId, key);
    ConsoleManager::instance().getConsoleFile(g_UserConsole)->dataIsReady();
}

void pedigree_event_return()
{
    // Return to the old code
    Processor::information().getScheduler().eventHandlerReturned();

    FATAL("event_return: should never get here");
}

void pedigree_input_register_callback(uintptr_t func)
{
    InputManager::instance().installCallback(InputManager::Key,
                                             reinterpret_cast<void (*)(uint64_t)>(func),
                                             Processor::information().getCurrentThread());
}

extern "C" void tuiModeChangedCallback()
{
    if (!g_UserConsole)
    {
        WARNING("Mode changed called with no console");
        return;
    }

    /// \todo Multiple displays.
    Config::Result *pResult = Config::instance().query("SELECT width,height FROM displays d, 'display-modes' m WHERE m.mode_id=d.mode_id");
    if (!pResult->succeeded() || !pResult->rows())
    {
        FATAL("Uh oh...");
    }
    uint64_t width  = static_cast<uint64_t> (pResult->getNum(0, "width"));
    uint64_t height = static_cast<uint64_t> (pResult->getNum(0, "height"));
    uint64_t a = (height<<32)|width;
    NOTICE("width: " << Hex << width << ", height: " << height << ", a: " << a);
    delete pResult;

    g_UserConsole->addAsyncRequest(0, TUI_MODE_CHANGED, g_UserConsoleId, a);
}

TuiSyscallManager::TuiSyscallManager() :
    m_pDisplay(0)
{
}

TuiSyscallManager::~TuiSyscallManager()
{
}

void TuiSyscallManager::initialise()
{
    m_pDisplay = g_pDisplay;

    SyscallManager::instance().registerSyscallHandler(TUI, this);
    // InputManager::instance().installCallback(InputManager::Key, callback);
}

uintptr_t TuiSyscallManager::call(uintptr_t function, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
    if (function >= serviceEnd)
    {
        ERROR("TuiSyscallManager: invalid function called: " << Dec << static_cast<int>(function));
        return 0;
    }
    return SyscallManager::instance().syscall(posix, function, p1, p2, p3, p4, p5);
}

uintptr_t TuiSyscallManager::syscall(SyscallState &state)
{
    uintptr_t p1 = state.getSyscallParameter(0);
    uintptr_t p2 = state.getSyscallParameter(1);
    uintptr_t p3 = state.getSyscallParameter(2);
    uintptr_t p4 = state.getSyscallParameter(3);
    uintptr_t p5 = state.getSyscallParameter(4);

    // We're interruptible.
    Processor::setInterrupts(true);

    switch (state.getSyscallNumber())
    {
        case TUI_NEXT_REQUEST:
            return g_UserConsole->nextRequest(p1, reinterpret_cast<char*>(p2), reinterpret_cast<size_t*>(p3), p4, reinterpret_cast<size_t*>(p5));
        case TUI_LOG:
        {
            // This is the solution to a bug - if the address in p1 traps (because of demand loading),
            // it MUST trap before we get the log spinlock, else other things will
            // want to write to it and deadlock.
            static char buf[1024];
            strncpy(buf, reinterpret_cast<char*>(p1), 1023);
            buf[1023] = '\0';
            NOTICE("TUI: " << buf);
            return 0;
        }
        case TUI_GETFB:
        {
            Display::ScreenMode *sm = reinterpret_cast<Display::ScreenMode*>(p1);
            *sm = g_ScreenMode;
            return 0;
        }
        case TUI_REQUEST_PENDING:
        {
            if (!g_UserConsole)
                WARNING("Request pending with no console");

            g_UserConsole->requestPending();
            return 0;
        }
        case TUI_RESPOND_TO_PENDING:
        {
            if (!g_UserConsole)
                WARNING("Respond to pending with no console");

            g_UserConsole->respondToPending(static_cast<size_t>(p1), reinterpret_cast<char*>(p2), static_cast<size_t>(p3));
            return 0;
        }
        case TUI_CREATE_CONSOLE:
        {
            size_t id = static_cast<size_t>(p1);
            char *name = reinterpret_cast<char*>(p2);

            UserConsole *pC = new UserConsole();
            ConsoleManager::instance().registerConsole(String(name), pC, id);
            g_Consoles[id] = pC;
            if (!g_UserConsole)
            {
                g_UserConsole = pC;
                g_UserConsoleId = id;
            }
            return 0;
        }
        case TUI_SET_CTTY:
        {
            char *name = reinterpret_cast<char*>(p1);

            Processor::information().getCurrentThread()->getParent()->setCtty(ConsoleManager::instance().getConsole(String(name)));
            break;
        }
        case TUI_SET_CURRENT_CONSOLE:
        {
            g_UserConsoleId = p1;
            g_UserConsole = g_Consoles[p1];
            break;
        }
        case TUI_VID_NEW_BUFFER:
        {
            if (!m_pDisplay) return 0;
            NOTICE("Getting new buffer.");
            return reinterpret_cast<uintptr_t> (m_pDisplay->newBuffer());
        }
        case TUI_VID_SET_BUFFER:
        {
            vid_req_t *pReq = reinterpret_cast<vid_req_t*>(p1);
            if (!m_pDisplay) return 0;

            m_pDisplay->setCurrentBuffer(reinterpret_cast<Display::rgb_t*>(p1));
            break;
        }
        case TUI_VID_UPDATE_BUFFER:
        {
            if (!m_pDisplay) return 0;
            vid_req_t *pReq = reinterpret_cast<vid_req_t*>(p1);

            m_pDisplay->updateBuffer(reinterpret_cast<Display::rgb_t*>(pReq->buffer), pReq->x, pReq->y, pReq->x2,
                                     pReq->y2);
            break;
        }
        case TUI_VID_KILL_BUFFER:
        {
            if (!m_pDisplay) return 0;

            m_pDisplay->killBuffer(reinterpret_cast<Display::rgb_t*>(p1));
            break;
        }
        case TUI_VID_BIT_BLIT:
        {
            if (!m_pDisplay) return 0;

            vid_req_t *pReq = reinterpret_cast<vid_req_t*>(p1);

            m_pDisplay->bitBlit(reinterpret_cast<Display::rgb_t*>(pReq->buffer), pReq->x, pReq->y, pReq->x2,
                                pReq->y2, pReq->w, pReq->h);
            break;
        }
        case TUI_VID_FILL_RECT:
        {
            if (!m_pDisplay) return 0;

            vid_req_t *pReq = reinterpret_cast<vid_req_t*>(p1);

            m_pDisplay->fillRectangle(reinterpret_cast<Display::rgb_t*>(pReq->buffer), pReq->x, pReq->y, pReq->w,
                                      pReq->h, *reinterpret_cast<Display::rgb_t*>(pReq->c));
            break;
        }
        case TUI_EVENT_RETURNED:
            pedigree_event_return();
            break;
        case TUI_INPUT_REGISTER_CALLBACK:
            pedigree_input_register_callback(static_cast<uintptr_t>(p1));
            break;
        default: ERROR ("TuiSyscallManager: invalid syscall received: " << Dec << state.getSyscallNumber()); return 0;
    }
    return 0;
}

void init()
{
    g_TuiSyscallManager.initialise();
}

void destroy()
{
}

MODULE_NAME("TUI");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
#ifdef X86_COMMON
// Why is uhci here?!
MODULE_DEPENDS("console", "uhci", "vbe");
#else
MODULE_DEPENDS("console", "uhci");
#endif
