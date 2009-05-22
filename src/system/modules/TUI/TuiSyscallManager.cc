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
#include <console/Console.h>
#include <Log.h>
#include <Module.h>

#include "TuiSyscallManager.h"
#include "Console.h"
#include "syscallNumbers.h"

UserConsole g_UserConsole;
TuiSyscallManager g_TuiSyscallManager;

TuiSyscallManager::TuiSyscallManager() :
    m_FramebufferRegion("Usermode framebuffer"), m_Mode()
{
}

TuiSyscallManager::~TuiSyscallManager()
{
}

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
    //uintptr_t p5 = state.getSyscallParameter(4);

    // We're interruptible.
    Processor::setInterrupts(true);

    switch (state.getSyscallNumber())
    {
        case TUI_NEXT_REQUEST:
            return g_UserConsole.nextRequest(p1, reinterpret_cast<char*>(p2), reinterpret_cast<size_t*>(p3), p4);
        case TUI_LOG:
            NOTICE("TUI: " << reinterpret_cast<char*>(p1));
            return 0;
        case TUI_GETFB:
        {
            Display::ScreenMode *pMode = reinterpret_cast<Display::ScreenMode*>(p1);
            *pMode = m_Mode;
            return reinterpret_cast<uintptr_t>(m_FramebufferRegion.virtualAddress());
        }
        default: ERROR ("TuiSyscallManager: invalid syscall received: " << Dec << state.getSyscallNumber()); return 0;
    }
}

void TuiSyscallManager::modeChanged(Display::ScreenMode mode, uintptr_t pFramebuffer, size_t pFbSize)
{
    m_Mode = mode;
    if (!PhysicalMemoryManager::instance().allocateRegion(m_FramebufferRegion,
                                                          pFbSize/PhysicalMemoryManager::getPageSize(),
                                                          PhysicalMemoryManager::nonRamMemory | PhysicalMemoryManager::force | PhysicalMemoryManager::continuous,
                                                          VirtualAddressSpace::Write,
                                                          pFramebuffer))
    {
        ERROR("TUI: Unable to create framebuffer memory region!");
    }                                          
}

void init()
{
    g_TuiSyscallManager.initialise();
    ConsoleManager::instance().registerConsole(String("console0"), &g_UserConsole, 0);
}

void destroy()
{
}

MODULE_NAME("TUI");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
MODULE_DEPENDS("console");
