/*
* Copyright (c) 2011 James Molloy, Jörg Pfähler, Matthew Iselin
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

#include <ipc/Ipc.h>
#include <native-ipc.h>

#include "NativeSyscallManager.h"
#include <nativeSyscallNumbers.h>

NativeSyscallManager::NativeSyscallManager()
{
}

NativeSyscallManager::~NativeSyscallManager()
{
}

void NativeSyscallManager::initialise()
{
    SyscallManager::instance().registerSyscallHandler(native, this);
}

uintptr_t NativeSyscallManager::call(uintptr_t function, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
    if (function >= serviceEnd)
    {
        ERROR("NativeSyscallManager: invalid function called: " << Dec << static_cast<int>(function));
        return 0;
    }
    return SyscallManager::instance().syscall(posix, function, p1, p2, p3, p4, p5);
}

uintptr_t NativeSyscallManager::syscall(SyscallState &state)
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
        case IPC_CREATE_STANDARD_MESSAGE:
            return createStandardMessage(reinterpret_cast<PedigreeIpc::IpcMessage*>(p1));
        case IPC_CREATE_SHARED_MESSAGE:
            return createSharedMessage(reinterpret_cast<PedigreeIpc::IpcMessage*>(p1), static_cast<size_t>(p2), p3);
        case IPC_GET_SHARED_REGION:
            return reinterpret_cast<uintptr_t>(getIpcSharedRegion(reinterpret_cast<PedigreeIpc::IpcMessage*>(p1)));
        case IPC_DESTROY_MESSAGE:
            destroyMessage(reinterpret_cast<PedigreeIpc::IpcMessage*>(p1));
            break;

        case IPC_SEND_IPC:
            return static_cast<uintptr_t>(sendIpc(reinterpret_cast<PedigreeIpc::IpcEndpoint*>(p1),reinterpret_cast<PedigreeIpc::IpcMessage*>(p2), static_cast<bool>(p3)));
        case IPC_RECV_PHASE1:
            return reinterpret_cast<uintptr_t>(recvIpcPhase1(reinterpret_cast<PedigreeIpc::IpcEndpoint*>(p1), static_cast<bool>(p2)));
        case IPC_RECV_PHASE2:
            return recvIpcPhase2(reinterpret_cast<PedigreeIpc::IpcMessage*>(p1), reinterpret_cast<void*>(p2));

        case IPC_CREATE_ENDPOINT:
            createEndpoint(reinterpret_cast<const char *>(p1));
            break;
        case IPC_REMOVE_ENDPOINT:
            removeEndpoint(reinterpret_cast<const char *>(p1));
            break;
        case IPC_GET_ENDPOINT:
            return reinterpret_cast<uintptr_t>(getEndpoint(reinterpret_cast<const char *>(p1)));
            break;

        default: ERROR ("NativeSyscallManager: invalid syscall received: " << Dec << state.getSyscallNumber()); return 0;
    }

    return 0;
}
