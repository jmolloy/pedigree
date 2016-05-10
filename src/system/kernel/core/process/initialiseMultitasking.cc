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

#include <Log.h>
#include <process/initialiseMultitasking.h>
#include <process/Thread.h>
#include <process/Scheduler.h>
#include <process/Process.h>
#include <processor/Processor.h>
#include <process/PerProcessorScheduler.h>
#include <process/RoundRobinCoreAllocator.h>
#include <process/ProcessorThreadAllocator.h>

void initialiseMultitasking()
{
  // Create the kernel idle process.
  Process *pProcess = new Process();
  pProcess->resetCounts();
  pProcess->description() += "Kernel Process";

#ifdef MULTIPROCESSOR
  pProcess->description() += " - Processor #";
  pProcess->description() += Processor::id();
#endif
  
  // Create the main kernel thread.
  Thread *pThread = new Thread(pProcess);
  pThread->detach();
  
  // Initialise the scheduler.
  Scheduler::instance().initialise(pProcess);

  // Initialise the per-process scheduler.
  Processor::information().getScheduler().initialise(pThread);
}

void shutdownMultitasking()
{
  /// \todo figure out how to shut down the scheduler then clean up the other
  /// housekeeping structures (including Process, Thread objects).
}

#ifdef MULTIPROCESSOR
  void initialiseMultitaskingPerProcessor()
  {
    // Create the kernel idle process.
    Process *pProcess = new Process();
    pProcess->description() += "Kernel Process";

    pProcess->description() += " - Processor #";
    pProcess->description() += Processor::id();

    // Create the kernel idle thread.
    Thread *pThread = new Thread(pProcess);
    pThread->detach();
    Processor::information().getScheduler().initialise(pThread);
  }
#endif

#endif
