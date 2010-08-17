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
#include <process/Scheduler.h>
#include <Log.h>

#include "PedigreeCSyscallManager.h"
#include "syscallNumbers.h"

#define PEDIGREEC_WITHIN_KERNEL
#include "pedigree-syscalls.h"

PedigreeCSyscallManager::PedigreeCSyscallManager()
{
}

PedigreeCSyscallManager::~PedigreeCSyscallManager()
{
}

void PedigreeCSyscallManager::initialise()
{
    SyscallManager::instance().registerSyscallHandler(pedigree_c, this);
    pedigree_config_init();
}

uintptr_t PedigreeCSyscallManager::call(uintptr_t function, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
    if (function >= serviceEnd)
    {
        ERROR("PedigreeCSyscallManager: invalid function called: " << Dec << static_cast<int>(function));
        return 0;
    }
    return SyscallManager::instance().syscall(posix, function, p1, p2, p3, p4, p5);
}

uintptr_t PedigreeCSyscallManager::syscall(SyscallState &state)
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
        // Pedigree system calls, called from POSIX applications
        case PEDIGREE_LOGIN:
            return pedigree_login(static_cast<int>(p1), reinterpret_cast<const char *>(p2));
        case PEDIGREE_LOAD_KEYMAP:
            return pedigree_load_keymap(reinterpret_cast<char*>(p1), static_cast<size_t>(p2));
        case PEDIGREE_GET_MOUNT:
            return pedigree_get_mount(reinterpret_cast<char*>(p1), reinterpret_cast<char*>(p2), static_cast<size_t>(p3));
        case PEDIGREE_REBOOT:
            pedigree_reboot();
            return 0;
        case PEDIGREE_CONFIG_GETCOLNAME:
            pedigree_config_getcolname(p1, p2, reinterpret_cast<char*>(p3), p4);
            return 0;
        case PEDIGREE_CONFIG_GETSTR_N:
            pedigree_config_getstr(p1, p2, reinterpret_cast<char*>(p3), p4);
            return 0;
        case PEDIGREE_CONFIG_GETSTR_S:
            pedigree_config_getstr(p1, reinterpret_cast<const char*>(p2), reinterpret_cast<char*>(p3), p4);
            return 0;
        case PEDIGREE_CONFIG_GETNUM_N:
            return pedigree_config_getnum(p1, p2);
        case PEDIGREE_CONFIG_GETNUM_S:
            return pedigree_config_getnum(p1, reinterpret_cast<const char*>(p2));
        case PEDIGREE_CONFIG_GETBOOL_N:
            return pedigree_config_getbool(p1, p2);
        case PEDIGREE_CONFIG_GETBOOL_S:
            return pedigree_config_getbool(p1, reinterpret_cast<const char*>(p2));
        case PEDIGREE_CONFIG_QUERY:
            return pedigree_config_query(reinterpret_cast<const char*>(p1));
        case PEDIGREE_CONFIG_FREERESULT:
            pedigree_config_freeresult(p1);
            return 0;
        case PEDIGREE_CONFIG_NUMCOLS:
            return pedigree_config_numcols(p1);
        case PEDIGREE_CONFIG_NUMROWS:
            return pedigree_config_numrows(p1);
        case PEDIGREE_CONFIG_NEXTROW:
            return pedigree_config_nextrow(p1);
        case PEDIGREE_CONFIG_WAS_SUCCESSFUL:
            return pedigree_config_was_successful(p1);
        case PEDIGREE_CONFIG_GET_ERROR_MESSAGE:
            pedigree_config_get_error_message(p1, reinterpret_cast<char*>(p2), p3);
            return 0;
        case PEDIGREE_MODULE_LOAD:
            pedigree_module_load(reinterpret_cast<char*>(p1));
            return 0;
        case PEDIGREE_MODULE_UNLOAD:
            pedigree_module_unload(reinterpret_cast<char*>(p1));
            return 0;
        case PEDIGREE_MODULE_IS_LOADED:
            return pedigree_module_is_loaded(reinterpret_cast<char*>(p1));
        case PEDIGREE_MODULE_GET_DEPENDING:
            return pedigree_module_get_depending(reinterpret_cast<char*>(p1), reinterpret_cast<char*>(p2), p3);
        case PEDIGREE_GFX_GET_PROVIDER:
            return pedigree_gfx_get_provider(reinterpret_cast<void*>(p1));
        case PEDIGREE_GFX_GET_CURR_MODE:
            return pedigree_gfx_get_curr_mode(reinterpret_cast<void*>(p1), reinterpret_cast<void*>(p2));
        case PEDIGREE_GFX_GET_RAW_BUFFER:
            return pedigree_gfx_get_raw_buffer(reinterpret_cast<void*>(p1));
        case PEDIGREE_GFX_CREATE_BUFFER:
            return pedigree_gfx_create_buffer(reinterpret_cast<void*>(p1), reinterpret_cast<void**>(p2), reinterpret_cast<void*>(p3));
        case PEDIGREE_GFX_DESTROY_BUFFER:
            return pedigree_gfx_destroy_buffer(reinterpret_cast<void*>(p1), reinterpret_cast<void*>(p2));
        case PEDIGREE_GFX_REDRAW:
            pedigree_gfx_redraw(reinterpret_cast<void*>(p1), p2, p3, p4, p5);
            return 0;
        case PEDIGREE_GFX_BLIT:
            pedigree_gfx_blit(reinterpret_cast<void*>(p1), reinterpret_cast<void*>(p2));
            return 0;
        case PEDIGREE_GFX_SET_PIXEL:
            pedigree_gfx_set_pixel(reinterpret_cast<void*>(p1), p2, p3, p4, p5);
            return 0;
        case PEDIGREE_GFX_RECT:
            pedigree_gfx_rect(reinterpret_cast<void*>(p1), reinterpret_cast<void*>(p2));
            return 0;
        case PEDIGREE_GFX_COPY:
            pedigree_gfx_copy(reinterpret_cast<void*>(p1), reinterpret_cast<void*>(p2));
            return 0;
        case PEDIGREE_GFX_LINE:
            pedigree_gfx_line(reinterpret_cast<void*>(p1), reinterpret_cast<void*>(p2));
            return 0;
        case PEDIGREE_GFX_DRAW:
            pedigree_gfx_draw(reinterpret_cast<void*>(p1), reinterpret_cast<void*>(p2));
            return 0;
        default: ERROR ("PedigreeCSyscallManager: invalid syscall received: " << Dec << state.getSyscallNumber()); return 0;
    }
}
