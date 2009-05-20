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

#if defined(THREADS)

#include <process/Process.h>
#include <processor/Processor.h>
#include <process/Scheduler.h>
#include <processor/VirtualAddressSpace.h>
#include <linker/Elf.h>
#include <processor/PhysicalMemoryManager.h>
#include <Log.h>

#include <process/SignalEvent.h>

Process::Process() :
  m_Threads(), m_NextTid(0), m_Id(0), str(), m_pParent(0), m_pAddressSpace(&VirtualAddressSpace::getKernelAddressSpace()),
  m_FdMap(), m_NextFd(0), m_FdLock(), m_ExitStatus(0), m_Cwd(0), m_SpaceAllocator(),
  m_pUser(0), m_pGroup(0), m_PendingSignals(), m_SignalHandlers(), m_SigReturnStub(0), m_SignalMask(0), m_SignalHandlersLock(false), m_PendingSignalsLock(false), m_pDynamicLinker(0), m_DeadThreads(0)
{
  m_Id = Scheduler::instance().addProcess(this);
  m_SpaceAllocator.free(0x00100000, 0x80000000); // Start off at 1MB so we never allocate 0x00000000 -
                                                 // This is treated as a "fail number" by the dynamic linker.
}


Process::Process(Process *pParent) :
  m_Threads(), m_NextTid(0), m_Id(0), str(), m_pParent(pParent), m_pAddressSpace(0), m_FdMap(), m_NextFd(0), m_FdLock(),
  m_ExitStatus(0), m_Cwd(pParent->m_Cwd), m_SpaceAllocator(pParent->m_SpaceAllocator), m_pUser(pParent->m_pUser), m_pGroup(pParent->m_pGroup),
  m_PendingSignals(pParent->m_PendingSignals), m_SignalHandlers(), m_SigReturnStub(pParent->m_SigReturnStub),
  m_SignalMask(pParent->m_SignalMask), m_SignalHandlersLock(false), m_PendingSignalsLock(false), m_pDynamicLinker(pParent->m_pDynamicLinker), m_DeadThreads(0)
{
  m_pAddressSpace = m_pParent->m_pAddressSpace->clone();

  m_Id = Scheduler::instance().addProcess(this);

  // Set a temporary description.
  str = m_pParent->str;
  str += "<F>"; // F for forked.

  // copy all signal handlers
  typedef Tree<size_t, void*> sigHandlerTree;
  sigHandlerTree &parentHandlers = pParent->m_SignalHandlers;
  sigHandlerTree &myHandlers = m_SignalHandlers;
  for(sigHandlerTree::Iterator it = parentHandlers.begin(); it != parentHandlers.end(); it++)
  {
    size_t key = reinterpret_cast<size_t>(it.key());
    void *value = it.value();

    SignalHandler *newSig = new SignalHandler(*reinterpret_cast<SignalHandler *>(value));
    myHandlers.insert(key, reinterpret_cast<void *>(newSig));
  }
}

Process::~Process()
{
  Scheduler::instance().removeProcess(this);
  Thread *pThread = m_Threads[0];
  m_Threads.erase(m_Threads.begin());
  delete pThread; // Calls Scheduler::remove and this::remove.

  Spinlock lock;
  lock.acquire(); // Disables interrupts.
  VirtualAddressSpace &VAddressSpace = Processor::information().getVirtualAddressSpace();

  Processor::switchAddressSpace(*m_pAddressSpace);
  m_pAddressSpace->revertToKernelAddressSpace();
  Processor::switchAddressSpace(VAddressSpace);

  delete m_pAddressSpace;
  lock.release();
}

size_t Process::addThread(Thread *pThread)
{
  m_Threads.pushBack(pThread);
  return (m_NextTid += 1);
}

void Process::removeThread(Thread *pThread)
{
  for(Vector<Thread*>::Iterator it = m_Threads.begin();
      it != m_Threads.end();
      it++)
  {
    if (*it == pThread)
    {
      m_Threads.erase(it);
      break;
    }
  }
}

size_t Process::getNumThreads()
{
  return m_Threads.count();
}

Thread *Process::getThread(size_t n)
{
  if (n >= m_Threads.count())
  {
    FATAL("Process::getThread(" << Dec << n << ") - Parameter out of bounds.");
    return 0;
  }
  return m_Threads[n];
}

void Process::kill()
{
  /// \todo Grab the scheduler lock!
  Processor::setInterrupts(false);

  NOTICE("Kill: " << m_Id << " (parent: " << m_pParent->getId() << ")");

  // Bye bye process - have we got any zombie children?
  for (size_t i = 0; i < Scheduler::instance().getNumProcesses(); i++)
  {
    Process *pProcess = Scheduler::instance().getProcess(i);

    if (pProcess->m_pParent == this)
    {
      if (pProcess->getThread(0)->getStatus() == Thread::Zombie)
      {
        // Kill 'em all!
        delete pProcess;
      }
      else
      {
        pProcess->m_pParent = 0;
      }
    }
  }

  // Kill all our threads except one, which exists in Zombie state.
  while (m_Threads.count() > 1)
  {
    Thread *pThread = m_Threads[0];
    if (pThread != Processor::information().getCurrentThread())
    {
        m_Threads.erase(m_Threads.begin());
        delete pThread; // Calls Scheduler::remove and this::remove.
    }
  }

  // Tell any threads that may be waiting for us to die.
  m_pParent->m_DeadThreads.release();

  Processor::information().getScheduler().schedule(Thread::Zombie);

  FATAL("Should never get here");
}

uintptr_t Process::create(uint8_t *elf, size_t elfSize, const char *name)
{
    FATAL("This function isn't implemented correctly - registration with the dynamic linker is required!");
  // At this point we're uninterruptible, as we're forking.
  Spinlock lock;
  lock.acquire();

  // Create a new process for the init process.
  Process *pProcess = new Process(Processor::information().getCurrentThread()->getParent());

  pProcess->description().clear();
  pProcess->description().append(name);

  VirtualAddressSpace &oldAS = Processor::information().getVirtualAddressSpace();

  // Switch to the init process' address space.
  Processor::switchAddressSpace(*pProcess->getAddressSpace());

  // That will have forked - we don't want to fork, so clear out all the chaff in the new address space that's not
  // in the kernel address space so we have a clean slate.
  pProcess->getAddressSpace()->revertToKernelAddressSpace();


   Elf initElf;
//   initElf.load(elf, elfSize);
//   uintptr_t iter = 0;
//   const char *lib = initElf.neededLibrary(iter);
//   initElf.allocateSegments();
//   initElf.writeSegments();

  for (int j = 0; j < 0x20000; j += 0x1000)
  {
    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    bool b = Processor::information().getVirtualAddressSpace().map(phys,
                                                                   reinterpret_cast<void*> (j+0x40000000),
                                                                   VirtualAddressSpace::Write);
    if (!b)
      WARNING("map() failed in init");
  }

  // Alrighty - lets create a new thread for this program - -8 as PPC assumes the previous stack frame is available...
  new Thread(pProcess, reinterpret_cast<Thread::ThreadStartFunc>(initElf.getEntryPoint()), 0x0 /* parameter */,  reinterpret_cast<void*>(0x40020000-8) /* Stack */);

  // Switch back to the old address space.
  Processor::switchAddressSpace(oldAS);

  lock.release();

  return pProcess->getId();
}

void Process::setSignalHandler(size_t sig, SignalHandler* handler)
{
    LockGuard<Mutex> guard(m_SignalHandlersLock);

    sig %= 32;
    if(handler)
    {
        SignalHandler* tmp;
        tmp = reinterpret_cast<SignalHandler*>(m_SignalHandlers.lookup(sig));
        if(tmp)
        {
            // Remove from the list
            m_SignalHandlers.remove(sig);

            // Destroy the SignalHandler struct
            delete tmp;
        }

        // Insert into the signal handler table
        handler->sig = sig;

        m_SignalHandlers.insert(sig, handler);
    }
}

#endif
