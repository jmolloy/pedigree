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

#include <pthread-syscalls.h>
#include <syscallError.h>
#include "errors.h"

extern "C"
{
    extern void pthread_stub();
    extern char pthread_stub_end;
}

int pthread_entry(void* fn)
{
    NOTICE("pthread child is starting");
    uintptr_t stack = reinterpret_cast<uintptr_t>(Processor::information().getVirtualAddressSpace().allocateStack());
    NOTICE("Stack = " << stack << ", fn = " << reinterpret_cast<uintptr_t>(fn) << ".");
    if(!stack)
        return -1;
    NOTICE("instructions at the trampoline: " << *reinterpret_cast<uint32_t*>(EVENT_HANDLER_TRAMPOLINE2) << "/" << *reinterpret_cast<uint32_t*>(EVENT_HANDLER_TRAMPOLINE) << "!");
    Processor::jumpUser(0, EVENT_HANDLER_TRAMPOLINE2, stack, reinterpret_cast<uintptr_t>(fn), 0, 0, 0);

    return 0;
}

int posix_pthread_create(pthread_t *thread, const pthread_attr_t *attr, pthreadfn start_addr, void *arg)
{
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    Thread *p = new Thread(pProcess, pthread_entry, reinterpret_cast<void*>(start_addr));
    return 0;
}

int posix_pthread_join(pthread_t thread, void **value_ptr)
{
    // Should wait for the thread to complete, and then put its return into value_ptr.
    return 0;
}

void posix_pthread_enter()
{
    PT_NOTICE("enter");
}

void posix_pthread_return(uintptr_t ret)
{
    PT_NOTICE("return(" << ret << ")");

    // Kill the thread
    Processor::information().getScheduler().killCurrentThread();

    while(1);
}

void pedigree_init_pthreads()
{
    PT_NOTICE("init_pthreads");

    // Map the signal return stub to the correct location
    /* This must be called after pedigree_init_sigret (which maps in a page anyway)
    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    Processor::information().getVirtualAddressSpace().map(phys,
            reinterpret_cast<void*> (EVENT_HANDLER_TRAMPOLINE2),
            VirtualAddressSpace::Write);
    */
    memcpy(reinterpret_cast<void*>(EVENT_HANDLER_TRAMPOLINE2), reinterpret_cast<void*>(pthread_stub), (reinterpret_cast<uintptr_t>(&pthread_stub_end) - reinterpret_cast<uintptr_t>(pthread_stub)));
    NOTICE("instructions at the trampoline: " << *reinterpret_cast<uint32_t*>(EVENT_HANDLER_TRAMPOLINE2) << "/" << *reinterpret_cast<uint32_t*>(EVENT_HANDLER_TRAMPOLINE) << "!");
}
