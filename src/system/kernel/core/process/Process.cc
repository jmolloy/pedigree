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
  m_ExitStatus(0), m_Cwd(0), m_Ctty(0), m_SpaceAllocator(true), m_pUser(0), m_pGroup(0), m_pEffectiveUser(0), m_pEffectiveGroup(0),
  m_pDynamicLinker(0), m_pSubsystem(0), m_DeadThreads(0)
{
  m_Id = Scheduler::instance().addProcess(this);
  m_SpaceAllocator.free(m_pAddressSpace->getUserStart(), m_pAddressSpace->getUserReservedStart());
}

Process::Process(Process *pParent) :
  m_Threads(), m_NextTid(0), m_Id(0), str(), m_pParent(pParent), m_pAddressSpace(0),
  m_ExitStatus(0), m_Cwd(pParent->m_Cwd), m_Ctty(pParent->m_Ctty), m_SpaceAllocator(pParent->m_SpaceAllocator),
  m_pUser(pParent->m_pUser), m_pGroup(pParent->m_pGroup), m_pEffectiveUser(pParent->m_pEffectiveUser), m_pEffectiveGroup(pParent->m_pEffectiveGroup),
  m_pDynamicLinker(pParent->m_pDynamicLinker), m_pSubsystem(0), m_DeadThreads(0)
{
   m_pAddressSpace = pParent->m_pAddressSpace->clone();
   // Copy the heap, but only if it's not the kernel heap (which is static)
  uintptr_t parentHeap = reinterpret_cast<uintptr_t>(pParent->m_pAddressSpace->m_Heap); // 0xc0000000
  if(parentHeap < m_pAddressSpace->getKernelStart()) /// \todo A better way would be nice.
    m_pAddressSpace->setHeap(pParent->m_pAddressSpace->m_Heap, pParent->m_pAddressSpace->m_HeapEnd);

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

  // Redraw on kill.
  /// \todo Caveat with redirection, maybe? Hmm...
  if(m_Ctty)
      m_Ctty->truncate();

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

  // Tell any threads that may be waiting for us to die.
  if (m_pParent)
      m_pParent->m_DeadThreads.release();
  else
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
                                                                   VirtualAddressSpace::Write| VirtualAddressSpace::Execute);
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

#endif
