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

#include "signal-syscalls.h"
#include "system-syscalls.h"
#include "file-syscalls.h"
#include <syscallError.h>
#include <Log.h>

#include <process/Scheduler.h>

/// \todo These are ok initially, but it'll all have to change at some point

#define SIGNAL_HANDLER_EXIT(name, errcode) void name(int s) { posix_exit(errcode); }
#define SIGNAL_HANDLER_EMPTY(name) void name(int s) {}
#define SIGNAL_HANDLER_EXITMSG(name, errcode, msg) void name(int s) { posix_write(2, msg, strlen(msg)); posix_exit(errcode); }

char SSIGILL[] = "Illegal instruction\n";
char SSIGSEGV[] = "Segmentation fault!\n";

SIGNAL_HANDLER_EXIT     (sigabrt, 1);
SIGNAL_HANDLER_EXIT     (sigalrm, 1);
SIGNAL_HANDLER_EXIT     (sigbus, 1);
SIGNAL_HANDLER_EMPTY    (sigchld);
SIGNAL_HANDLER_EMPTY    (sigcont); /// \todo Continue & Pause execution
SIGNAL_HANDLER_EXIT     (sigfpe, 1); // floating point exception signal
SIGNAL_HANDLER_EXIT     (sighup, 1);
SIGNAL_HANDLER_EXITMSG  (sigill, 1, SSIGILL);
SIGNAL_HANDLER_EXIT     (sigint, 1);
SIGNAL_HANDLER_EXIT     (sigkill, 1);
SIGNAL_HANDLER_EXIT     (sigpipe, 1);
SIGNAL_HANDLER_EXIT     (sigquit, 1);
SIGNAL_HANDLER_EXITMSG  (sigsegv, 1, SSIGSEGV);
SIGNAL_HANDLER_EMPTY    (sigstop); /// \todo Continue & Pause execution
SIGNAL_HANDLER_EXIT     (sigterm, 1);
SIGNAL_HANDLER_EMPTY    (sigtstp); // terminal stop
SIGNAL_HANDLER_EMPTY    (sigttin); // background process attempts read
SIGNAL_HANDLER_EMPTY    (sigttou); // background process attempts write
SIGNAL_HANDLER_EMPTY    (sigusr1);
SIGNAL_HANDLER_EMPTY    (sigusr2);
SIGNAL_HANDLER_EMPTY    (sigurg); // high bandwdith data available at a sockeet

SIGNAL_HANDLER_EMPTY    (sigign);

_sig_func_ptr default_sig_handlers[] =
{
                          sigign, // null signal
                          sighup,
                          sigint,
                          sigquit,
                          sigill,
                          sigign, // no SIGTRAP
                          sigign, // no SIGIOT
                          sigabrt,
                          sigign, // no SIGEMT
                          sigfpe,
                          sigkill,
                          sigbus,
                          sigsegv,
                          sigign, // no SIGSYS
                          sigpipe,
                          sigalrm,
                          sigterm,
                          sigurg,
                          sigstop,
                          sigtstp,
                          sigcont,
                          sigchld,
                          sigign, // no SIGCLD
                          sigttin,
                          sigttou,
                          sigign, // no SIGIO
                          sigign, // no SIGXCPU
                          sigign, // no SIGXFSZ
                          sigign, // no SIGVTALRM
                          sigign, // no SIGPROF
                          sigign, // no SIGWINCH,
                          sigign, // no SIGLOST
                          sigusr1,
                          sigusr2
};

// useful macros from sys/signal.h
#define sigaddset(what,sig) (*(what) |= (1<<(sig)), 0)
#define sigdelset(what,sig) (*(what) &= ~(1<<(sig)), 0)
#define sigemptyset(what)   (*(what) = 0, 0)
#define sigfillset(what)    (*(what) = ~(0), 0)
#define sigismember(what,sig) (((*(what)) & (1<<(sig))) != 0)

int posix_sigaction(int sig, const struct sigaction *act, struct sigaction *oact, int type)
{
  SC_NOTICE("sigaction(" << Dec << sig << Hex << ")");

  Thread* pThread = Processor::information().getCurrentThread();
  Process* pProcess = pThread->getParent();

  // sanity and safety checks
  if((sig > 32) || (sig == SIGKILL || sig == SIGSTOP))
  {
    SYSCALL_ERROR(InvalidArgument);
    return -1;
  }
  sig %= 32;

  // store the old signal handler information if we can
  if(oact)
  {
    Process::SignalHandler* oldSignalHandler = pProcess->getSignalHandler(sig);
    if(oldSignalHandler)
    {
      oact->sa_flags = oldSignalHandler->flags;
      oact->sa_mask = oldSignalHandler->sigMask;
      if(oldSignalHandler->type == 0)
        oact->sa_handler = reinterpret_cast<void (*)(int)>(oldSignalHandler->handlerLocation);
      else if(oldSignalHandler->type == 1)
        oact->sa_handler = reinterpret_cast<void (*)(int)>(0);
      else if(oldSignalHandler->type == 2)
        oact->sa_handler = reinterpret_cast<void (*)(int)>(1);
    }
    else
      memset(oact, 0, sizeof(struct sigaction));
  }

  // and if needed, fill in the new signal handler
  if(act)
  {
    Process::SignalHandler* sigHandler = new Process::SignalHandler;
    sigHandler->handlerLocation = reinterpret_cast<uintptr_t>(act->sa_handler);
    sigHandler->sigMask = act->sa_mask;
    sigHandler->flags = act->sa_flags;

    uintptr_t newHandler = reinterpret_cast<uintptr_t>(act->sa_handler);
    if(newHandler == 0)
    {
      sigHandler->handlerLocation = reinterpret_cast<uintptr_t>(default_sig_handlers[sig]);
      sigHandler->type = 1;
    }
    else if(newHandler == 1)
    {
      sigHandler->handlerLocation = reinterpret_cast<uintptr_t>(sigign);
      sigHandler->type = 2;
    }
    else if(static_cast<int>(newHandler) == -1)
    {
      delete sigHandler;
      SYSCALL_ERROR(InvalidArgument);
      return -1;
    }
    else
    {
      sigHandler->handlerLocation = newHandler;
      sigHandler->type = 0;
    }

    pProcess->setSignalHandler(sig, sigHandler);
  }
  else if(!oact)
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

int posix_raise(int sig)
{
  SC_NOTICE("raise");

  // create the pending signal and pass it in
  Process* pProcess = Processor::information().getCurrentThread()->getParent();
  Process::SignalHandler* signalHandler = pProcess->getSignalHandler(sig);
  Process::PendingSignal* pendingSignal = new Process::PendingSignal;
  if(signalHandler)
    pendingSignal->sigMask = signalHandler->sigMask;

  // add the signal to the queue
  pProcess->addPendingSignal(static_cast<size_t>(sig), pendingSignal);

  // let another thread be scheduled; we won't come back until after the pending signal is handled
  Scheduler::instance().yield();

  return 0;
}

int pedigree_sigret()
{
  SC_NOTICE("pedigree_sigret");

  // we do not want an interrupt until we're waiting for the signal handler to complete
  bool bInterrupts = Processor::getInterrupts();
  if(bInterrupts)
    Processor::setInterrupts(false);

  Thread* pThread = Processor::information().getCurrentThread();

  // set the parent signal mask back the way it was
  pThread->getParent()->setSignalMask(pThread->getCurrentSignal().oldMask);

  // reset the thread information
  Thread::CurrentSignal curr;
  pThread->setCurrentSignal(curr);

  // when we reschedule we will go to the saved state rather than the state picked up
  // when this thread is switched *from*
//  pThread->useSaved(true);

  Processor::information().getScheduler().signalHandlerReturned();

  // and now that we're done fiddling with the thread let us be interrupted
  if(bInterrupts)
    Processor::setInterrupts(true);

  // reschedule, when we get back to this thread it'll load the old state
  Scheduler::instance().yield();
  while(1)
    Processor::halt();

  return 0;
}

int posix_kill(int pid, int sig)
{
  SC_NOTICE("kill(" << pid << ", " << sig << ")");

  Process* p = Scheduler::instance().getProcess(static_cast<size_t>(pid));
  if(p)
  {
    if((p->getSignalMask() & (1 << sig)))
    {
      /// \todo What happens when the signal is blocked?
      return 0;
    }

    // build the pending signal and pass it in
    Process::SignalHandler* signalHandler = p->getSignalHandler(sig);
    Process::PendingSignal* pendingSignal = new Process::PendingSignal;
    pendingSignal->sigMask = signalHandler->sigMask;

    // add the signal to the queue
    p->addPendingSignal(static_cast<size_t>(sig), pendingSignal);
  }
  else
  {
    SYSCALL_ERROR(NoSuchProcess);
    return -1;
  }

  return 0;
}

int posix_sigprocmask(int how, const uint32_t *set, uint32_t *oset)
{
  SC_NOTICE("sigprocmask");

  uint32_t currMask = Processor::information().getCurrentThread()->getParent()->getSignalMask();

  if(oset)
  {
    // if no actual passed set, the how argument is invalid and we merely return the current mask
    *oset = currMask;
    if(!set)
      return 0;
  }
  if(!set)
  {
    SYSCALL_ERROR(InvalidArgument);
    return -1;
  }

  uint32_t passedMask = *set;

  // SIGKILL and SIGSTOP are not blockable
  sigdelset(&passedMask, 9);
  sigdelset(&passedMask, 17);

  uint32_t returnMask = 0;
  bool bProcessed = false;
  switch(how)
  {
    // SIG_BLOCK: union of the current set and the passed set
    case 0:
      returnMask = currMask | passedMask;
      bProcessed = true;
      break;
     // SIG_SETMASK: set the mask to the passed set
    case 1:
      returnMask = passedMask;
      bProcessed = true;
      break;
    // SIG_UNBLOCK: unset the bits in the passed set
    case 2:
      returnMask = currMask ^ passedMask;
      bProcessed = true;
      break;
  };

  if(!bProcessed)
  {
    SYSCALL_ERROR(InvalidArgument);
    return -1;
  }
  else
  {
    Processor::information().getCurrentThread()->getParent()->setSignalMask(returnMask);

    // now that the new signal mask is set, reschedule so that any (now unblocked) signals may run
    Scheduler::instance().yield();
  }

  return 0;
}

int posix_alarm(uint32_t seconds)
{
  /// \todo Implement
  SC_NOTICE("alarm");
  return 0;
}

int posix_sleep(uint32_t seconds)
{
  /// \todo Implement
  SC_NOTICE("sleep");
  return 0;
}
