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

#include "signal-syscalls.h"
#include "system-syscalls.h"
#include "file-syscalls.h"
#include "pthread-syscalls.h"
#include <syscallError.h>
#include <Log.h>

#include <process/Scheduler.h>
#include <processor/PhysicalMemoryManager.h>

#define MACHINE_FORWARD_DECL_ONLY
#include <machine/Machine.h>
#include <machine/Timer.h>

#include <Subsystem.h>
#include <PosixSubsystem.h>
#include <PosixProcess.h>

extern "C"
{
    extern void sigret_stub();
    extern char sigret_stub_end;
}

int doProcessKill(Process *p, int sig);

/// \todo These are ok initially, but it'll all have to change at some point

#define SIGNAL_HANDLER_EXIT(name, errcode) void name(int s) { posix_exit(errcode); }
#define SIGNAL_HANDLER_EMPTY(name) void name(int s) {NOTICE("EMPTY");}
#define SIGNAL_HANDLER_EXITMSG(name, errcode, msg) void name(int s) { Processor::setInterrupts(true); posix_write(1, msg, strlen(msg), true); Scheduler::instance().yield(); posix_exit(errcode); }
#define SIGNAL_HANDLER_SUSPEND(name) void name(int s) { NOTICE("SUSPEND"); Process *pParent = Processor::information().getCurrentThread()->getParent()->getParent(); Processor::information().getCurrentThread()->getParent()->suspend(); }
#define SIGNAL_HANDLER_RESUME(name) void name(int s) { NOTICE("RESUME"); Processor::information().getCurrentThread()->getParent()->resume(); Process *pParent = Processor::information().getCurrentThread()->getParent()->getParent(); }

char SSIGILL[] = "Illegal instruction.\n";
char SSIGSEGV[] = "Segmentation fault.\n";
char SSIGBUS[] = "Bus error.\n";
char SSIGABRT[] = "Abort.\n";

SIGNAL_HANDLER_EXITMSG  (sigabrt, SIGABRT, SSIGABRT)
SIGNAL_HANDLER_EXIT     (sigalrm, SIGALRM)
SIGNAL_HANDLER_EXITMSG  (sigbus, SIGBUS, SSIGBUS)
SIGNAL_HANDLER_EMPTY    (sigchld)
SIGNAL_HANDLER_RESUME   (sigcont)
SIGNAL_HANDLER_EXIT     (sigfpe, SIGFPE) // floating point exception signal
SIGNAL_HANDLER_EXIT     (sighup, SIGHUP)
SIGNAL_HANDLER_EXITMSG  (sigill, SIGILL, SSIGILL)
SIGNAL_HANDLER_EXIT     (sigint, SIGINT)
SIGNAL_HANDLER_EXIT     (sigkill, SIGKILL)
SIGNAL_HANDLER_EXIT     (sigpipe, SIGPIPE)
SIGNAL_HANDLER_EXIT     (sigquit, SIGQUIT)
SIGNAL_HANDLER_EXITMSG  (sigsegv, SIGSEGV, SSIGSEGV)
SIGNAL_HANDLER_SUSPEND  (sigstop)
SIGNAL_HANDLER_EXIT     (sigterm, SIGTERM)
SIGNAL_HANDLER_SUSPEND  (sigtstp) // terminal stop
SIGNAL_HANDLER_SUSPEND  (sigttin) // background process attempts read
SIGNAL_HANDLER_SUSPEND  (sigttou) // background process attempts write
SIGNAL_HANDLER_EMPTY    (sigusr1)
SIGNAL_HANDLER_EMPTY    (sigusr2)
SIGNAL_HANDLER_EMPTY    (sigurg) // high bandwdith data available at a sockeet

SIGNAL_HANDLER_EMPTY    (sigign);

_sig_func_ptr default_sig_handlers[32] =
{
    sigign, // null signal = 0
    sighup, // SIGHUP = 1
    sigint, // SIGINT = 2
    sigquit, // SIGQUIT = 3
    sigill, // SIGILL = 4
    sigign, // no SIGTRAP = 5
    sigabrt, // SIGABRT = 6
    sigign, // no SIGEMT = 7
    sigfpe, // SIGFPE = 8
    sigkill, // SIGKILL = 9
    sigbus, // SIGBUS = 10
    sigsegv, // SIGSEGV = 11
    sigign, // no SIGSYS = 12
    sigpipe, // SIGPIPE = 13
    sigalrm, // SIGALRM = 14
    sigterm, // SIGTERM = 15
    sigurg, // SIGURG = 16
    sigstop, // SIGSTOP = 17
    sigtstp, // SIGTSTP = 18
    sigcont, // SIGCONT = 19
    sigchld, // SIGCHLD = 20
    sigttin, // SIGTTIN = 21
    sigttou, // SIGTTOU = 22
    sigign, // no SIGIO = 23
    sigign, // no SIGXCPU = 24
    sigign, // no SIGXFSZ = 25
    sigign, // no SIGVTALRM = 26
    sigign, // no SIGPROF = 27
    sigign, // no SIGWINCH = 28
    sigign, // no SIGLOST = 29
    sigusr1, // SIGUSR1 = 30
    sigusr2 // SIGUSR2 = 31
};

int posix_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    SG_NOTICE("sigaction(" << Dec << sig << Hex << ", " << reinterpret_cast<uintptr_t>(act) << ", " << reinterpret_cast<uintptr_t>(oact) << ")");
    if((act && !PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(act), sizeof(struct sigaction), PosixSubsystem::SafeRead)) ||
        (oact && !PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(oact), sizeof(struct sigaction), PosixSubsystem::SafeWrite)))
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if(!pSubsystem)
    {
        ERROR("posix_sigaction: no subsystem");
        return -1;
    }

    // sanity and safety checks
    if ((sig > 32) || (sig == SIGKILL || sig == SIGSTOP))
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }
    sig %= 32;

    // store the old signal handler information if we can
    if (oact)
    {
        PosixSubsystem::SignalHandler* oldSignalHandler = pSubsystem->getSignalHandler(sig);
        if (oldSignalHandler)
        {
            oact->sa_flags = oldSignalHandler->flags;
            oact->sa_mask = oldSignalHandler->sigMask;
            if (oldSignalHandler->type == 0)
                oact->sa_handler = reinterpret_cast<void (*)(int)>(oldSignalHandler->pEvent->getHandlerAddress());
            else if (oldSignalHandler->type == 1)
                oact->sa_handler = reinterpret_cast<void (*)(int)>(0);
            else if (oldSignalHandler->type == 2)
                oact->sa_handler = reinterpret_cast<void (*)(int)>(1);
        }
        else
            memset(oact, 0, sizeof(struct sigaction));
    }

    // and if needed, fill in the new signal handler
    if (act)
    {
        PosixSubsystem::SignalHandler* sigHandler = new PosixSubsystem::SignalHandler;
        sigHandler->sigMask = act->sa_mask;
        sigHandler->flags = act->sa_flags;

        uintptr_t newHandler = reinterpret_cast<uintptr_t>(act->sa_handler);
        if (newHandler == 0)
        {
            SG_VERBOSE_NOTICE(" + SIG_DFL");
            newHandler = reinterpret_cast<uintptr_t>(default_sig_handlers[sig]);
            sigHandler->type = 1;
        }
        else if (newHandler == 1)
        {
            SG_VERBOSE_NOTICE(" + SIG_IGN");
            newHandler = reinterpret_cast<uintptr_t>(sigign);
            sigHandler->type = 2;
        }
        else if (static_cast<int>(newHandler) == -1)
        {
            SG_VERBOSE_NOTICE(" + Invalid");
            delete sigHandler;
            SYSCALL_ERROR(InvalidArgument);
            return -1;
        }
        else
        {
            // SG_NOTICE(" + <handler has been provided>");
            sigHandler->type = 0;
        }

        size_t nLevel = pThread->getStateLevel();
        sigHandler->pEvent = new SignalEvent(newHandler, static_cast<size_t>(sig)); //, nLevel);
        SG_NOTICE("Creating the event (" << reinterpret_cast<uintptr_t>(sigHandler->pEvent) << ").");
        pSubsystem->setSignalHandler(sig, sigHandler);
    }
    else if (!oact)
    {
        // no valid arguments!
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }
    return 0;
}

uintptr_t posix_signal(int sig, void* func)
{
    ERROR("signal called but glue signal should redirect to sigaction");
    return 0;
}

int posix_raise(int sig, SyscallState &State)
{
    SG_NOTICE("raise");

    // Create the pending signal and pass it in
    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if(!pSubsystem)
    {
        ERROR("posix_raise: no subsystem");
        return -1;
    }

    PosixSubsystem::SignalHandler* signalHandler = pSubsystem->getSignalHandler(sig);

    // Firing and checking the event state needs to be done without any interrupts
    // getting in the way.
    bool bWasInterrupts = Processor::getInterrupts();
    Processor::setInterrupts(false);

    // Fire the event, and wait for it to complete
    if (signalHandler->pEvent)
        pThread->sendEvent(reinterpret_cast<Event*>(signalHandler->pEvent));

    // If the alternate stack is available, and not in use, use that
    uintptr_t stackPointer = State.getStackPointer();
    PosixSubsystem::AlternateSignalStack &currStack = pSubsystem->getAlternateSignalStack();
    if(currStack.enabled && !currStack.inUse)
        stackPointer = (currStack.base + currStack.size) - 1;

    // Jump to the signal handler
    Processor::information().getScheduler().checkEventState(stackPointer);
    Processor::setInterrupts(bWasInterrupts);

    // All done
    return 0;
}

int pedigree_sigret()
{
    SG_NOTICE("pedigree_sigret");

    // Grab the subsystem for this thread
    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());

    // If the alternate stack is in use, we're done with it for now
    PosixSubsystem::AlternateSignalStack &currStack = pSubsystem->getAlternateSignalStack();
    if(currStack.inUse)
        currStack.inUse = false;

    // Return to the old code
    Processor::information().getScheduler().eventHandlerReturned();

    FATAL("eventHandlerReturned() returned!");

    return 0;
}

void pedigree_unwind_signal()
{
    SG_NOTICE("pedigree_unwind_signal");
    
    // Pop a state from the thread, but don't jump to it.
    Thread *pThread = Processor::information().getCurrentThread();
    pThread->popState();
}

int doThreadKill(Thread *p, int sig)
{
    // Are we allowed to do this?
    if(p->getStatus() == Thread::Suspended)
    {
        if(!(sig == SIGKILL || sig == SIGCONT))
        {
            WARNING("kill: can't send anything other than SIGKILL or SIGCONT to a suspended process.");
            return -1;
        }
    }

    Process *pThisProcess = Processor::information().getCurrentThread()->getParent();

    // Build the pending signal and pass it in
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(p->getParent()->getSubsystem());
    if(!pSubsystem)
    {
        ERROR("posix_kill: no subsystem on process " << p->getParent()->getId());
        return -1;
    }
    PosixSubsystem::SignalHandler* signalHandler = pSubsystem->getSignalHandler(sig);
    if (signalHandler && signalHandler->pEvent)
    {
        // Fire the event
        p->sendEvent(reinterpret_cast<Event*>(signalHandler->pEvent));

        // Don't schedule to the process if that process is us.
        if (p->getParent() != pThisProcess)
        {
            // Switch to that context in order to handle the event
            bool bWasInterrupts = Processor::getInterrupts();
            Processor::setInterrupts(false);
            Processor::information().getScheduler().schedule(Thread::Ready, p);
            Processor::setInterrupts(bWasInterrupts);
        }
    }

    return 0;
}

int doProcessKill(Process *p, int sig)
{
    return doThreadKill(p->getThread(0), sig);
}

int posix_kill(int pid, int sig)
{
    SG_NOTICE("kill(" << pid << ", " << sig << ")");

    List<Process *> processList;

    // Metadata about the calling process.
    PosixProcess *pThisProcess = static_cast<PosixProcess *>(Processor::information().getCurrentThread()->getParent());
    ProcessGroup *pThisGroup = pThisProcess->getProcessGroup();

    bool bKillingSelf = false;

    // Check for the process(es) we are about to kill.
    size_t i = 0;
    for(; i < Scheduler::instance().getNumProcesses(); ++i)
    {
        Process *pProcess = Scheduler::instance().getProcess(i);

        if (pProcess->getThread(0)->getStatus() == Thread::Zombie)
        {
            // Oops, process already been terminated.
            if (static_cast<int>(pProcess->getId()) == pid)
                break;
            else
                continue;
        }
        else if ((pid <= 0) && (pProcess->getType() == Process::Posix))
        {
            PosixProcess *pPosixProcess = static_cast<PosixProcess *>(pProcess);
            ProcessGroup *pGroup = pPosixProcess->getProcessGroup();
            if (pid == 0)
            {
                // Any process in the same process group as the caller.
                if (!(pGroup && pThisGroup))
                    continue;
                if(pGroup->processGroupId != pThisGroup->processGroupId)
                    continue;
            }
            else if (pid == -1)
            {
                // Kill all processes we have permission to kill (limit to only direct children for now)
                if(pProcess->getParent() != pThisProcess)
                    continue;
            }
            else if(pGroup && (pGroup->processGroupId != (pid * -1)))
            {
                // Absolute group ID reference
                continue;
            }
        }
        else if ((pid > 0) && (static_cast<int>(pProcess->getId()) != pid))
            continue;
        else if (pProcess->getType() != Process::Posix)
            continue;

        // Okay, the process is good.
        processList.pushBack(pProcess);
    }

    // No process(es) found?
    if(processList.count() == 0)
    {
        SYSCALL_ERROR(NoSuchProcess);
        SG_NOTICE("  -> no such process");
        return -1;
    }

    // Go ahead and kill each process.
    for(List<Process *>::Iterator it = processList.begin();
        it != processList.end();
        ++it)
    {
        PosixProcess *member = static_cast<PosixProcess *>(*it);
        if(member != pThisProcess)
            doProcessKill(member, sig);
        else
            bKillingSelf = true;
    }

    // Yield to allow the events to be propagated across the process(es)
    Scheduler::instance().yield();

    if(bKillingSelf)
    {
        doProcessKill(pThisProcess, sig);

        // If it was us, try to handle the signal *now*, or else we're going to end up who-knows-where on return.
        Processor::information().getScheduler().checkEventState(0);
    }

    return 0;
}

int posix_pthread_kill(pthread_t thread, int sig)
{
    PT_NOTICE("pthread_kill");

    // Check the signal
    if(sig > 32)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Grab the subsystem
    Thread *pCurrThread = Processor::information().getCurrentThread();
    Process *pProcess = pCurrThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Grab the target thread, send the signal
    PosixSubsystem::PosixThread *pTarget = pSubsystem->getThread(thread->__internal.kthread);
    if(pTarget && pTarget->pThread)
    {
        return doThreadKill(pTarget->pThread, sig);
    }
    else
    {
        SYSCALL_ERROR(NoSuchProcess);
        return -1;
    }
}

/// \todo Integration with Thread inhibit masks
int posix_sigprocmask(int how, const uint32_t *set, uint32_t *oset)
{
    return 0;
}

int posix_pthread_sigmask(int how, const uint32_t *set, uint32_t *oset)
{
    return 0;
}

size_t posix_alarm(uint32_t seconds)
{
    SG_NOTICE("alarm");

    // Create the pending signal and pass it in
    Process* pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if(!pSubsystem)
    {
        ERROR("posix_alarm: no subsystem");
        return -1;
    }

    PosixSubsystem::SignalHandler* signalHandler = pSubsystem->getSignalHandler(SIGALRM);
    Event *pEvent = signalHandler->pEvent;

    // We now have the Event...
    if(seconds == 0)
    {
        // Cancel the previous alarm, return the time it still had to go
        if (pEvent)
            return Machine::instance().getTimer()->removeAlarm(pEvent, false);
        return 0;
    }
    else
    {
        if (pEvent)
        {
            // Stop any previous alarm
            size_t ret = Machine::instance().getTimer()->removeAlarm(pEvent, false);

            // Install the new one
            Machine::instance().getTimer()->addAlarm(pEvent, seconds);

            // Return the time the previous event still had to go
            return ret;
        }
    }

    // All done
    return 0;
}

int posix_sleep(uint32_t seconds)
{
    SG_NOTICE("sleep");

    Semaphore sem(0);

    uint64_t startTick = Machine::instance().getTimer()->getTickCount();
    sem.acquire(1, seconds);
    if (Processor::information().getCurrentThread()->wasInterrupted())
    {
        // Note: seconds is a uint32, therefore it should be safe to cast
        // to half the width
        uint64_t endTick = Machine::instance().getTimer()->getTickCount();
        uint64_t elapsed = endTick - startTick;
        uint32_t elapsedSecs = static_cast<uint32_t>(elapsed / 1000) + 1;

        if(elapsedSecs >= seconds)
            return 0;
        else
            return elapsedSecs;
    }
    else
    {
        ERROR("sleep: acquire was not interrupted?");
    }

    return 0;
}

int posix_usleep(size_t useconds)
{
    SG_NOTICE("usleep");

    Semaphore sem(0);

    size_t seconds = useconds / 1000000;
    useconds %= 1000000;

    sem.acquire(1, seconds, useconds);
    if (Processor::information().getCurrentThread()->wasInterrupted())
    {
        return 0;
    }
    else
    {
        ERROR("usleep: acquire was not interrupted?");
    }

    return -1;
}

int posix_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    if((!PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(rqtp), sizeof(struct timespec), PosixSubsystem::SafeRead)) ||
        (rmtp && !PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(rmtp), sizeof(struct timespec), PosixSubsystem::SafeWrite)))
    {
        SG_NOTICE("nanosleep -> invalid address");
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    SG_NOTICE("nanosleep(" << Dec << rqtp->tv_sec << ":" << rqtp->tv_nsec << Hex << ") - " << Machine::instance().getTimer()->getTickCount() << ".");

    Semaphore sem(0);

    uint64_t startTick = Machine::instance().getTimer()->getTickCount();
    sem.acquire(1, rqtp->tv_sec, rqtp->tv_nsec / 1000);
    if (Processor::information().getCurrentThread()->wasInterrupted())
    {
        uint64_t endTick = Machine::instance().getTimer()->getTickCount();
        uint64_t elapsed = endTick - startTick;
        if(rmtp)
        {
            rmtp->tv_nsec = static_cast<time_t>(static_cast<uint64_t>(elapsed * 1000) % 1000000000ULL);
            rmtp->tv_sec = static_cast<time_t>(elapsed / 1000);
        }
        
        /// \todo Handle "interrupted before end of timeout"
        Processor::information().getCurrentThread()->setInterrupted(false);
        return 0;
    }
    else
    {
        ERROR("sleep: acquire was not interrupted?");
    }

    return 0;
}

int posix_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    SG_NOTICE("clock_gettime");
    if(!PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(tp), sizeof(struct timespec), PosixSubsystem::SafeWrite))
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }
    
    // All clocks are equal, but some are more equal than others.
    // Seriously though, we don't currently care about the id value.
    
    // We only care about the nanoseconds that may have passed in the past
    // second - everything else is handled by the UNIX timestamp.
    tp->tv_nsec = static_cast<time_t>(static_cast<uint64_t>((Machine::instance().getTimer()->getTickCount() * 1000)) % 1000000000ULL);
    tp->tv_sec = static_cast<time_t>(Machine::instance().getTimer()->getUnixTimestamp());
    
    return 0;
}

int posix_sigaltstack(const struct stack_t *stack, struct stack_t *oldstack)
{
    /// \todo Check stacks are sane (checkAddress).

    // Verify arguments
    if(!stack && !oldstack)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }
    if(stack && (stack->ss_size < MINSIGSTKSZ))
    {
        SYSCALL_ERROR(OutOfMemory);
        return -1;
    }

    // Grab the subsystem for this thread
    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());

    // Look at the current alternative stack
    PosixSubsystem::AlternateSignalStack &currStack = pSubsystem->getAlternateSignalStack();

    // Are we running on the alternate stack?
    if(currStack.inUse)
    {
        SG_NOTICE("Can't set new alternate signal stack as it's the one we're running on!");
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }
    
    // Fill the old stack, if needed
    if(oldstack)
    {
        oldstack->ss_sp = reinterpret_cast<void*>(currStack.base);
        oldstack->ss_size = currStack.size;
        oldstack->ss_flags = (currStack.enabled ? 0 : SA_DISABLE) | (currStack.inUse ? SA_ONSTACK : 0);
    }

    // Set the new one
    if(stack)
    {
        currStack.base = reinterpret_cast<uintptr_t>(stack->ss_sp);
        currStack.size = stack->ss_size;
        currStack.enabled = (stack->ss_flags & SA_DISABLE) != SA_DISABLE;
    }

    // Success!
    return 0;
}

void pedigree_init_sigret()
{
    SG_NOTICE("init_sigret");
    static physical_uintptr_t sigretPhys = 0;

    // Handle allocation if needed.
    if(sigretPhys == 0)
    {
        sigretPhys = PhysicalMemoryManager::instance().allocatePage();
        PhysicalMemoryManager::instance().pin(sigretPhys);

        // Map trampoline page in and bring across the sigret code.
        Processor::information().getVirtualAddressSpace().map(
                sigretPhys,
                reinterpret_cast<void*> (EVENT_HANDLER_TRAMPOLINE),
                VirtualAddressSpace::Write | VirtualAddressSpace::Shared | VirtualAddressSpace::Execute);

        memcpy(reinterpret_cast<void*>(EVENT_HANDLER_TRAMPOLINE), reinterpret_cast<void*>(sigret_stub), (reinterpret_cast<uintptr_t>(&sigret_stub_end) - reinterpret_cast<uintptr_t>(sigret_stub)));

        // Mark read-only now that we have mapped in the page.
        Processor::information().getVirtualAddressSpace().setFlags(
                reinterpret_cast<void*> (EVENT_HANDLER_TRAMPOLINE),
                VirtualAddressSpace::Execute | VirtualAddressSpace::Shared);
    }

    // Map the signal return stub to the correct location
    if(!Processor::information().getVirtualAddressSpace().isMapped(reinterpret_cast<void *>(EVENT_HANDLER_TRAMPOLINE)))
    {
        Processor::information().getVirtualAddressSpace().map(
                sigretPhys,
                reinterpret_cast<void*> (EVENT_HANDLER_TRAMPOLINE),
                VirtualAddressSpace::Shared | VirtualAddressSpace::Execute);
    }

    // Install default signal handlers
    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if(!pSubsystem)
    {
        pSubsystem = new PosixSubsystem();
        pProcess->setSubsystem(pSubsystem);
        pSubsystem->setProcess(pProcess);
    }

    for(size_t i = 0; i < 32; i++)
    {
        // Constructor zeroes out everything, which is correct for this initial
        // setup of the signal handlers (except, of course, the handler location).
        PosixSubsystem::SignalHandler* sigHandler = new PosixSubsystem::SignalHandler;
        sigHandler->sig = i;
        sigHandler->type = 1; // SIG_DFL - default handler now.

        uintptr_t newHandler = reinterpret_cast<uintptr_t>(default_sig_handlers[i]);

        size_t nLevel = pThread->getStateLevel();
        sigHandler->pEvent = new SignalEvent(newHandler, i); //, nLevel);

        pSubsystem->setSignalHandler(i, sigHandler);
    }

    SG_NOTICE("Creating initial set of signal handlers is complete");
}
