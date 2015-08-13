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

#include <process/Scheduler.h>
#include <pthread-syscalls.h>
#include <syscallError.h>
#include "errors.h"
#include "PosixSubsystem.h"

extern "C"
{
    extern void pthread_stub();
    extern char pthread_stub_end;
}

struct pthreadInfoBlock
{
    void *entry;
    void *arg;
    void *stack;
} __attribute__((packed));

/**
 * pthread_kernel_enter: Kernel side entry point for all POSIX userspace threads.
 *
 * Excuse the "story" littered throughout this (and other) function(s).
 * When you're working with pthreads, sometimes you need small things to
 * lighten up your coding, and this is one way that works (so far).
 *
 * \param fn A pointer to a pthreadInfoBlock structure that contains information
 *           about the entry point and arguments for the new thread.
 * \return Nothing. Had you going for a moment there with that int, didn't I? If
 *         this function does return, kindly ignore the resulting fatal error
 *         and look for the flying pigs.
 */
int pthread_kernel_enter(void *blk)
{
    PT_NOTICE("pthread_kernel_enter");

    // Grab our backpack with our map and additional information
    pthreadInfoBlock *args = reinterpret_cast<pthreadInfoBlock*>(blk);
    void *entry = args->entry;
    void *new_args = args->arg;
    void *new_stack = args->stack;

    // It's grabbed now, so we don't need any more room on the bag rack.
    delete args;

    PT_NOTICE("pthread child [" << reinterpret_cast<uintptr_t>(entry) << "] is starting");

    // Just keep using our current stack, don't bother to save any state. We'll
    // never return, so there's no need to be sentimental (and no need to worry
    // about return addresses etc)
    uintptr_t stack = reinterpret_cast<uintptr_t>(new_stack) - sizeof(uintptr_t);
    if(!stack) // Because sanity costs little.
        return -1;

    // Begin our quest from the Kernel Highlands and dive deep into the murky
    // depths of the Userland River. We'll take with us our current stack, a
    // map to our destination, and some additional information that may come
    // in handy when we arrive.
    Processor::jumpUser(0, EVENT_HANDLER_TRAMPOLINE2, stack, reinterpret_cast<uintptr_t>(entry), reinterpret_cast<uintptr_t>(new_args), 0, 0);

    // If we get here, we probably got catapaulted many miles back to the
    // Kernel Highlands. That's probably not a good thing. They won't really
    // accept us here anymore :(
    FATAL("I'm not supposed to be in the Kernel Highlands!");
    return 0;
}

/**
 * posix_pthread_create: POSIX thread creation
 *
 * This function will create a new execution thread in userspace.
 */
int posix_pthread_create(pthread_t *thread, const pthread_attr_t *attr, pthreadfn start_addr, void *arg)
{
    PT_NOTICE("pthread_create");

    // Build some defaults for the attributes, if needed
    size_t stackSize = 0;
    if(attr)
    {
        if(attr->__internal.stackSize >= PTHREAD_STACK_MIN)
            stackSize = attr->__internal.stackSize;
    }

    // Grab the subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Allocate a stack for this thread
    void *stack = pProcess->getAddressSpace()->allocateStack(stackSize);
    if(!stack)
    {
        ERROR("posix_pthread_create: couldn't get a stack!");
        SYSCALL_ERROR(OutOfMemory);
        return -1;
    }

    // Build the information structure to pass to the pthread entry function
    pthreadInfoBlock *dat = new pthreadInfoBlock;
    dat->entry = reinterpret_cast<void*>(start_addr);
    dat->arg = arg;
    dat->stack = stack;

    // Create the thread
    Thread *pThread = new Thread(pProcess, pthread_kernel_enter, reinterpret_cast<void*>(dat), 0, true);

    // Create our information structure, shove in the initial elements
    PosixSubsystem::PosixThread *p = new PosixSubsystem::PosixThread;
    p->pThread = pThread;
    p->returnValue = 0;

    // Take information from the attributes
    if(attr)
    {
        p->isDetached = (attr->__internal.detachState == PTHREAD_CREATE_DETACHED);
    }

    // Insert the thread
    pSubsystem->insertThread(pThread->getId(), p);
    thread->__internal.kthread = pThread->getId();

    // All done!
    return 0;
}

/**
 * posix_pthread_join: POSIX thread join
 *
 * Joining a thread basically means the caller of pthread_join will wait until
 * the passed thread completes execution. The return value from the thread is
 * stored in value_ptr.
 */
int posix_pthread_join(pthread_t *thread, void **value_ptr)
{
    PT_NOTICE("pthread_join");

    // Grab the subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Grab the thread information structure and verify it
    PosixSubsystem::PosixThread *p = pSubsystem->getThread(thread->__internal.kthread);
    if(!p)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Check that we can join
    if(p->isDetached)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Now that we have it, wait for the thread and then store the return value
    p->isRunning.acquire();
    if(value_ptr)
        *value_ptr = p->returnValue;

    // Clean up - we're never going to use this again
    pSubsystem->removeThread(thread->__internal.kthread);
    delete p;

    // Success!
    return 0;
}

/**
 * posix_pthread_detach: POSIX thread detach
 *
 * If the thread has completed, clean up its information. Otherwise signal that
 * its thread information will not be used, and can be cleaned up.
 *
 * Similar to join, but without caring about the return value.
 */
int posix_pthread_detach(pthread_t *thread)
{
    PT_NOTICE("pthread_detach");

    // Grab the subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Grab the thread information structure and verify it
    PosixSubsystem::PosixThread *p = pSubsystem->getThread(thread->__internal.kthread);
    if(!p)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Check that we can detach
    if(p->isDetached)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Now that we have it, wait for the thread and then store the return value
    if(p->isRunning.tryAcquire())
    {
        // Clean up - we're never going to use this again
        pSubsystem->removeThread(thread->__internal.kthread);
        delete p;
    }
    else
        p->canReclaim = true;

    // Success!
    return 0;
}

/**
 * kernel_pthread_self: Returns the thread ID for the current thread.
 */
static size_t kernel_pthread_self()
{
    PT_NOTICE("kernel_pthread_self");
    Thread *pThread = Processor::information().getCurrentThread();
    PT_NOTICE("    -> " << Dec << pThread->getId() << Hex);
    return pThread->getId();
}

/**
 * posix_pthread_enter: Called by userspace when entering the new thread.
 *
 * At this stage, this function does not do anything. In future, this may
 * change to support adding new state information to the thread lists, or
 * to set some kernel-side state.
 */
int posix_pthread_enter(uintptr_t arg)
{
    PT_NOTICE("pthread_enter");
    return 0;
}

/**
 * posix_pthread_exit
 *
 * This function is called either when the thread function returns, or
 * when pthread_exit is called. The single parameter specifies a return
 * value from the thread.
 */
void posix_pthread_exit(void *ret)
{
    PT_NOTICE("pthread_exit");

    // Grab the subsystem and unlock any waiting threads
    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (pSubsystem)
    {
        PosixSubsystem::PosixThread *p = pSubsystem->getThread(pThread->getId());
        if(p)
        {
            p->returnValue = ret;
            p->isRunning.release();

            if(p->canReclaim)
            {
                // Clean up...
                pSubsystem->removeThread(pThread->getId());
                delete p;
            }
        }
    }

    // Kill the thread
    Processor::information().getScheduler().killCurrentThread();

    while(1);
}

/**
 * pedigree_init_pthreads
 *
 * This function copies the user mode thread wrapper from the kernel to a known
 * user mode location. The location is already mapped by pedigree_init_signals
 * which must be called before this function.
 */
void pedigree_init_pthreads()
{
    PT_NOTICE("init_pthreads");
    // Make sure we can write to the trampoline area.
    Processor::information().getVirtualAddressSpace().setFlags(
            reinterpret_cast<void*> (EVENT_HANDLER_TRAMPOLINE),
            VirtualAddressSpace::Write);
    memcpy(reinterpret_cast<void*>(EVENT_HANDLER_TRAMPOLINE2), reinterpret_cast<void*>(pthread_stub), (reinterpret_cast<uintptr_t>(&pthread_stub_end) - reinterpret_cast<uintptr_t>(pthread_stub)));
    Processor::information().getVirtualAddressSpace().setFlags(
            reinterpret_cast<void*> (EVENT_HANDLER_TRAMPOLINE),
            VirtualAddressSpace::Execute | VirtualAddressSpace::Shared);

    // Make sure the main thread is actually known.
    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if(!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return;
    }

    PosixSubsystem::PosixThread *p = new PosixSubsystem::PosixThread;
    p->pThread = pThread;
    p->returnValue = 0;
    pSubsystem->insertThread(pThread->getId(), p);
}

void* posix_pthread_getspecific(pthread_key_t *key)
{
    PT_NOTICE("pthread_getspecific");
    
    // Grab the subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if(!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return 0;
    }

    // Grab the current thread id
    size_t threadId = kernel_pthread_self();
    PosixSubsystem::PosixThread *pThread = pSubsystem->getThread(threadId);
    if(!pThread)
    {
        ERROR("Not a valid POSIX thread?");
        return 0;
    }

    // Grab the data
    PosixSubsystem::PosixThreadKey *data = pThread->getThreadData(key->__internal.key);
    if(!data)
    {
        SYSCALL_ERROR(InvalidArgument);
        return 0;
    }

    // Return it
    return data->buffer;
}

int posix_pthread_setspecific(pthread_key_t *key, const void *buff)
{
    PT_NOTICE("pthread_setspecific");
    
    // Grab the subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if(!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Grab the current thread id
    size_t threadId = kernel_pthread_self();
    PosixSubsystem::PosixThread *pThread = pSubsystem->getThread(threadId);
    if(!pThread)
    {
        ERROR("Not a valid POSIX thread?");
        return -1;
    }

    // Grab the data
    PosixSubsystem::PosixThreadKey *data = pThread->getThreadData(key->__internal.key);
    if(!data)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Set it
    data->buffer = const_cast<void*>(buff);

    return 0;
}

int posix_pthread_key_create(pthread_key_t *okey, key_destructor destructor)
{
    PT_NOTICE("pthread_key_create");
    
    // Grab the subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if(!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Grab the current thread id
    size_t threadId = kernel_pthread_self();
    PosixSubsystem::PosixThread *pThread = pSubsystem->getThread(threadId);
    if(!pThread)
    {
        ERROR("Not a valid POSIX thread?");
        return -1;
    }

    // Grab the next available key
    size_t key = ~0UL;
    for(size_t i = pThread->lastDataKey; i < pThread->nextDataKey; i++)
    {
        if(!pThread->m_ThreadKeys.test(i))
        {
            pThread->lastDataKey = i;
            pThread->m_ThreadKeys.set(i);
            key = i;
            break;
        }
    }
    if(key == ~0UL)
    {
        key = pThread->nextDataKey;
        pThread->m_ThreadKeys.set(pThread->nextDataKey);
        pThread->nextDataKey++;
    }

    // Create the data key
    PosixSubsystem::PosixThreadKey *data = new PosixSubsystem::PosixThreadKey;
    data->destructor = destructor;
    data->buffer = 0;
    pThread->addThreadData(key, data);

    // Success!
    okey->__internal.key = key;
    return 0;
}

key_destructor posix_pthread_key_destructor(pthread_key_t *key)
{
    PT_NOTICE("pthread_key_destructor");
    
    // Grab the subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if(!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return 0;
    }

    // Grab the current thread id
    size_t threadId = kernel_pthread_self();
    PosixSubsystem::PosixThread *pThread = pSubsystem->getThread(threadId);
    if(!pThread)
    {
        ERROR("Not a valid POSIX thread?");
        return 0;
    }

    // Grab the data
    PosixSubsystem::PosixThreadKey *data = pThread->getThreadData(key->__internal.key);
    if(!data)
    {
        SYSCALL_ERROR(InvalidArgument);
        return 0;
    }

    return data->destructor;
}

int posix_pthread_key_delete(pthread_key_t *key)
{
    PT_NOTICE("pthread_key_delete");
    
    // Grab the subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if(!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Grab the current thread id
    size_t threadId = kernel_pthread_self();
    PosixSubsystem::PosixThread *pThread = pSubsystem->getThread(threadId);
    if(!pThread)
    {
        ERROR("Not a valid POSIX thread?");
        return -1;
    }

    // Grab the data
    size_t ikey = key->__internal.key;
    PosixSubsystem::PosixThreadKey *data = pThread->getThreadData(ikey);
    if(!data)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Remove the key from the thread
    pThread->removeThreadData(ikey);

    // It's now safe to let other calls use this key
    pThread->lastDataKey = ikey;
    pThread->m_ThreadKeys.clear(ikey);

    // Destroy it
    delete data;

    return 0;
}

void *posix_pedigree_create_waiter()
{
    Semaphore *sem = new Semaphore(0);
    return reinterpret_cast<void *>(sem);
}

int posix_pedigree_thread_wait_for(void *waiter)
{
    Semaphore *sem = reinterpret_cast<Semaphore *>(waiter);
    
    // Deadlock detection - don't wait if nothing can wake this waiter.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    if (pProcess->getNumThreads() <= 1)
    {
        SYSCALL_ERROR(Deadlock);
        return -1;
    }

    while(!sem->acquire(1));

    return 0;
}

int posix_pedigree_thread_trigger(void *waiter)
{
    Semaphore *sem = reinterpret_cast<Semaphore *>(waiter);
    if (sem->getValue())
        return 0;  // Nothing to wake up.

    // Wake up a waiter.
    sem->release();
    return 1;
}

void posix_pedigree_destroy_waiter(void *waiter)
{
    Semaphore *sem = reinterpret_cast<Semaphore *>(waiter);
    delete sem;
}
