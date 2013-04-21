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
#include "tuiSyscallNumbers.h"

// UserConsole *g_Consoles[256];
// UserConsole *g_UserConsole = 0;
//size_t g_UserConsoleId = 0;
TuiSyscallManager g_TuiSyscallManager;

/// Current console, by PID.
Tree<uint64_t, UserConsole *> g_UserConsoleMap;

// All consoles within a process, by PID.
Tree<uint64_t, Tree<uintptr_t, UserConsole *>* > g_ConsolesMap;

void TuiSyscallManager::initialise()
{
    SyscallManager::instance().registerSyscallHandler(TUI, this);
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
    
    static uintptr_t currBuff = 0;

    // Set locals, if any.
    uint64_t pid = Processor::information().getCurrentThread()->getParent()->getId();
    UserConsole *g_UserConsole = g_UserConsoleMap.lookup(pid);
    Tree<uintptr_t, UserConsole *> *g_Consoles = g_ConsolesMap.lookup(pid);

    if(!g_Consoles)
    {
        g_Consoles = new Tree<uintptr_t, UserConsole *>;
        g_ConsolesMap.insert(pid, g_Consoles);
    }

    switch (state.getSyscallNumber())
    {
        case TUI_NEXT_REQUEST:
        case TUI_NEXT_REQUEST_ASYNC:
            if (!g_UserConsole)
            {
                WARNING("Next request with no console");
                return 0;
            }
            else
                return g_UserConsole->nextRequest(p1, reinterpret_cast<char*>(p2), reinterpret_cast<size_t*>(p3), p4, reinterpret_cast<size_t*>(p5), state.getSyscallNumber() == TUI_NEXT_REQUEST_ASYNC);
        case TUI_REQUEST_PENDING:
        {
            if (!g_UserConsole)
                WARNING("Request pending with no console");
            else
                g_UserConsole->requestPending();
            return 0;
        }
        case TUI_RESPOND_TO_PENDING:
        {
            if (!g_UserConsole)
                WARNING("Respond to pending with no console");
            else
                g_UserConsole->respondToPending(static_cast<size_t>(p1), reinterpret_cast<char*>(p2), static_cast<size_t>(p3));
            return 0;
        }
        case TUI_CREATE_CONSOLE:
        {
            size_t id = static_cast<size_t>(p1);
            char *name = reinterpret_cast<char*>(p2);

            UserConsole *pC = new UserConsole();
            ConsoleManager::instance().registerConsole(String(name), pC, id);
            g_Consoles->insert(id, pC);
            if (!g_UserConsole)
            {
                g_UserConsoleMap.insert(pid, pC);
                // g_UserConsoleId = id;
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
            if (g_UserConsole)
            {
                g_UserConsole->stopCurrentBlock();
                g_UserConsoleMap.remove(pid);
            }
            
            //g_UserConsoleId = p1;
            g_UserConsole = g_Consoles->lookup(p1);
            g_UserConsoleMap.insert(pid, g_UserConsole);
            break;
        }
        case TUI_STOP_REQUEST_QUEUE:
            if (!g_UserConsole)
                WARNING("Request queue tried to stop without a console");
            else
                g_UserConsole->stopCurrentBlock();
            break;

        case TUI_DATA_CHANGED:
        {
            char *name = reinterpret_cast<char*>(p1);
            ConsoleFile *pConsole = ConsoleManager::instance().getConsoleFile(g_UserConsole);
            pConsole->dataIsReady();
            break;
        }
        default: ERROR ("TuiSyscallManager: invalid syscall received: " << Dec << state.getSyscallNumber() << Hex); return 0;
    }
    return 0;
}

static void init()
{
    g_TuiSyscallManager.initialise();
}

static void destroy()
{
}

MODULE_INFO("TUI", &init, &destroy, "console", "gfx-deps");
