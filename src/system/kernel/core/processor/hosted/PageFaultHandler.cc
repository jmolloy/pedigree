/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include <Log.h>
#include <Debugger.h>
#include <processor/PageFaultHandler.h>
#include <process/Scheduler.h>
#include <panic.h>
#include <processor/PhysicalMemoryManager.h>
#include "VirtualAddressSpace.h"

namespace __pedigree_hosted {};
using namespace __pedigree_hosted;

#include <signal.h>
#include <ucontext.h>

PageFaultHandler PageFaultHandler::m_Instance;

bool PageFaultHandler::initialise()
{
    InterruptManager &IntManager = InterruptManager::instance();
    bool r1 = IntManager.registerInterruptHandler(SIGSEGV, this);
    bool r2 = IntManager.registerInterruptHandler(SIGBUS, this);

    return r1 && r2;
}

void PageFaultHandler::interrupt(size_t interruptNumber, InterruptState &state)
{
    siginfo_t *info = reinterpret_cast<siginfo_t *>(state.getRegister(1));

    uintptr_t page = reinterpret_cast<uintptr_t>(info->si_addr);
    uintptr_t code = info->si_code;

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    if (va.isMapped(reinterpret_cast<void*>(page)))
    {
        physical_uintptr_t phys;
        size_t flags;
        va.getMapping(reinterpret_cast<void*>(page), phys, flags);
        if (flags & VirtualAddressSpace::CopyOnWrite)
        {
#ifdef SUPERDEBUG
            NOTICE_NOLOCK(Processor::information().getCurrentThread()->getParent()->getId() << " PageFaultHandler: copy-on-write for v=" << page);
#endif

            Process *pProcess = Processor::information().getCurrentThread()->getParent();
            size_t pageSz = PhysicalMemoryManager::instance().getPageSize();

            // Get a temporary page in which we can store the current mapping for copy.
            uintptr_t tempAddr = 0;
            pProcess->getSpaceAllocator().allocate(pageSz, tempAddr);

            // Map temporary page to the old page.
            if (!va.map(phys, reinterpret_cast<void *>(tempAddr), VirtualAddressSpace::KernelMode))
            {
                FATAL("PageFaultHandler: CoW temporary map() failed");
                return;
            }

            // OK, we can now unmap the old page - we hold a valid temporary mapping.
            va.unmap(reinterpret_cast<void*>(page));

            // Allocate new page for the new memory region.
            physical_uintptr_t p = PhysicalMemoryManager::instance().allocatePage();
            if (!p)
            {
                FATAL("PageFaultHandler: CoW OOM'd!");
                return;
            }

            // Map in the new page, making sure to mark it not CoW.
            flags |= VirtualAddressSpace::Write;
            flags &= ~VirtualAddressSpace::CopyOnWrite;
            if (!va.map(p, reinterpret_cast<void*>(page), flags))
            {
                FATAL("PageFaultHandler: CoW new map() failed.");
                return;
            }

            // Perform the actual copy.
            memcpy(reinterpret_cast<uint8_t*>(page),
                   reinterpret_cast<uint8_t*>(tempAddr),
                   pageSz);

            // Release temporary page.
            va.unmap(reinterpret_cast<void *>(tempAddr));
            pProcess->getSpaceAllocator().free(tempAddr, pageSz);

            // Clean up old reference to memory (may free the page, if we were the
            // last one to reference the CoW page)
            PhysicalMemoryManager::instance().freePage(phys);
            return;
        }
    }

    if (page < reinterpret_cast<uintptr_t>(KERNEL_SPACE_START))
    {
        // Check our handler list.
        for (List<MemoryTrapHandler*>::Iterator it = m_Handlers.begin();
                it != m_Handlers.end();
                it++)
        {
            if ((*it)->trap(page, code == SEGV_ACCERR))
            {
                return;
            }
        }
    }

    // Extra information comes from the ucontext_t structure passed to the
    // signal handler (SIGSEGV).
    uintptr_t ucontext_loc = state.getRegister(2);
    ucontext_t *ctx = reinterpret_cast<ucontext_t *>(ucontext_loc);
    state.setInstructionPointer(ctx->uc_mcontext.gregs[REG_RIP]);
    state.setStackPointer(ctx->uc_mcontext.gregs[REG_RSP]);
    state.setBasePointer(ctx->uc_mcontext.gregs[REG_RBP]);

    //  Get PFE location and error code
    static LargeStaticString sError;
    sError.clear();
    sError.append("Page Fault Exception at 0x");
    sError.append(page, 16, 8, '0');
    sError.append(", error code 0x");
    sError.append(code, 16, 8, '0');
    sError.append(", EIP 0x");
    sError.append(state.getInstructionPointer(), 16, 8, '0');

    //  Extract error code information
    static LargeStaticString sCode;
    sCode.clear();
    sCode.append("Details: PID=");
    sCode.append(Processor::information().getCurrentThread()->getParent()->getId());
    sCode.append(" ");

    if (code == SEGV_MAPERR) sCode.append("NOT ");
    sCode.append("PRESENT | ");

    ERROR(static_cast<const char*>(sError));
    ERROR(static_cast<const char*>(sCode));

#ifdef DEBUGGER
    if(state.kernelMode())
        Debugger::instance().start(state, sError);
#endif

    Scheduler &scheduler = Scheduler::instance();
    if (UNLIKELY(scheduler.getNumProcesses() == 0))
    {
        //  We are in the early stages of the boot process (no processes started)
        panic(sError);
    }
    else
    {
        //  Unrecoverable PFE in a process - Kill the process and yield
        //Processor::information().getCurrentThread()->getParent()->kill();
        Thread *pThread = Processor::information().getCurrentThread();
        Process *pProcess = pThread->getParent();
        Subsystem *pSubsystem = pProcess->getSubsystem();
        if (pSubsystem)
            pSubsystem->threadException(pThread, Subsystem::PageFault);
        else
        {
            pProcess->kill();

            //  kill member function also calls yield(), so shouldn't get here.
            for (;;) ;
        }
    }
}

PageFaultHandler::PageFaultHandler() :
    m_Handlers()
{
}
