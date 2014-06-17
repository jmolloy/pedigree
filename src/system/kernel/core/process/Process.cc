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

#if defined(THREADS)

#include <processor/types.h>
#include <process/Process.h>
#include <processor/Processor.h>
#include <process/Scheduler.h>
#include <processor/VirtualAddressSpace.h>
#include <linker/Elf.h>
#include <processor/PhysicalMemoryManager.h>
#include <Log.h>
#include <utilities/ZombieQueue.h>

#include <process/SignalEvent.h>

#include <Subsystem.h>

#include <vfs/File.h>

Process::Process() :
  m_Threads(), m_NextTid(0), m_Id(0), str(), m_pParent(0), m_pAddressSpace(&VirtualAddressSpace::getKernelAddressSpace()),
  m_ExitStatus(0), m_Cwd(0), m_Ctty(0), m_SpaceAllocator(false), m_DynamicSpaceAllocator(false),
  m_pUser(0), m_pGroup(0), m_pEffectiveUser(0), m_pEffectiveGroup(0), m_pDynamicLinker(0),
  m_pSubsystem(0), m_Waiters(), m_bUnreportedSuspend(false), m_bUnreportedResume(false),
  m_State(Active), m_BeforeSuspendState(Thread::Ready), m_DeadThreads(0)
{
  m_Id = Scheduler::instance().addProcess(this);
  getSpaceAllocator().free(
      getAddressSpace()->getUserStart(),
      getAddressSpace()->getUserReservedStart() - getAddressSpace()->getUserStart());
  if(getAddressSpace()->getDynamicStart())
  {
    getDynamicSpaceAllocator().free(
        getAddressSpace()->getDynamicStart(),
        getAddressSpace()->getDynamicEnd() - getAddressSpace()->getDynamicStart());
  }
}

Process::Process(Process *pParent) :
  m_Threads(), m_NextTid(0), m_Id(0), str(), m_pParent(pParent), m_pAddressSpace(0),
  m_ExitStatus(0), m_Cwd(pParent->m_Cwd), m_Ctty(pParent->m_Ctty),
  m_SpaceAllocator(pParent->m_SpaceAllocator), m_DynamicSpaceAllocator(pParent->m_DynamicSpaceAllocator),
  m_pUser(pParent->m_pUser), m_pGroup(pParent->m_pGroup), m_pEffectiveUser(pParent->m_pEffectiveUser),
  m_pEffectiveGroup(pParent->m_pEffectiveGroup), m_pDynamicLinker(pParent->m_pDynamicLinker),
  m_pSubsystem(0), m_Waiters(), m_bUnreportedSuspend(false), m_bUnreportedResume(false),
  m_State(pParent->getState()), m_BeforeSuspendState(Thread::Ready), m_DeadThreads(0)
{
   m_pAddressSpace = pParent->m_pAddressSpace->clone();

  m_Id = Scheduler::instance().addProcess(this);
 
  // Set a temporary description.
  str = m_pParent->str;
  str += "<F>"; // F for forked.
}

Process::~Process()
{
  Scheduler::instance().removeProcess(this);
  Thread *pThread = m_Threads[0];
  if(m_Threads.count())
    m_Threads.erase(m_Threads.begin());
  else
      WARNING("Process with an empty thread list, potentially unstable situation");
  if(!pThread && m_Threads.count())
      WARNING("Process with a null entry for the first thread, potentially unstable situation");
  if (pThread != Processor::information().getCurrentThread())
      delete pThread; // Calls Scheduler::remove and this::remove.
  else if(pThread)
      pThread->setParent(0);
  if(m_pSubsystem)
    delete m_pSubsystem;

  VirtualAddressSpace &VAddressSpace = Processor::information().getVirtualAddressSpace();

  bool bInterrupts = Processor::getInterrupts();
  Processor::setInterrupts(false);

  Processor::switchAddressSpace(*m_pAddressSpace);
  m_pAddressSpace->revertToKernelAddressSpace();
  Processor::switchAddressSpace(VAddressSpace);

  delete m_pAddressSpace;

  Processor::setInterrupts(bInterrupts);
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
    FATAL("Process::getThread(" << Dec << n << Hex << ") - Parameter out of bounds.");
    return 0;
  }
  return m_Threads[n];
}

void Process::kill()
{
  /// \todo Grab the scheduler lock!
  Processor::setInterrupts(false);

  if(m_pParent)
	NOTICE("Kill: " << m_Id << " (parent: " << m_pParent->getId() << ")");
  else
	NOTICE("Kill: " << m_Id << " (parent: <orphan>)");

  // Bye bye process - have we got any zombie children?
  for (size_t i = 0; i < Scheduler::instance().getNumProcesses(); i++)
  {
    Process *pProcess = Scheduler::instance().getProcess(i);

    if (pProcess && (pProcess->m_pParent == this))
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

  // Add to the zombie queue if the process is an orphan.
  if (!m_pParent)
  {
      NOTICE("Adding process to zombie queue for cleanup");
      
      ZombieQueue::instance().addObject(new ZombieProcess(this));
      Processor::information().getScheduler().killCurrentThread();
      
      // Should never get here.
      FATAL("Process: should never get here");
  }

  // We'll get reaped elsewhere
  NOTICE("Not adding process to zombie queue for cleanup");
  Processor::information().getScheduler().schedule(Thread::Zombie);

  FATAL("Should never get here");
}

void Process::suspend()
{
    m_bUnreportedSuspend = true;
    m_ExitStatus = 0x7F;
    m_BeforeSuspendState = m_Threads[0]->getStatus();
    m_State = Suspended;
    notifyWaiters();
    // Notify parent that we're suspending.
    if(m_pParent && m_pParent->getSubsystem())
        m_pParent->getSubsystem()->threadException(m_pParent->getThread(0), Subsystem::Child);
    Processor::information().getScheduler().schedule(Thread::Suspended);
}

void Process::resume()
{
    m_bUnreportedResume = true;
    m_ExitStatus = 0xFF;
    m_State = Active;
    notifyWaiters();
    Processor::information().getScheduler().schedule(Thread::Ready);
}

uintptr_t Process::create(uint8_t *elf, size_t elfSize, const char *name)
{
    FATAL("This function isn't implemented correctly - registration with the dynamic linker is required!");
    return 0;
}

void Process::addWaiter(Semaphore *pWaiter)
{
    m_Waiters.pushBack(pWaiter);
}

void Process::removeWaiter(Semaphore *pWaiter)
{
    for(List<Semaphore *>::Iterator it = m_Waiters.begin();
        it != m_Waiters.end();
        )
    {
        if((*it) == pWaiter)
        {
            it = m_Waiters.erase(it);
        }
        else
            ++it;
    }
}

void Process::notifyWaiters()
{
    for(List<Semaphore *>::Iterator it = m_Waiters.begin();
        it != m_Waiters.end();
        ++it)
    {
        (*it)->release();
    }
}

#endif
