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

#include <Log.h>
#include <Debugger.h>
#include "PageFaultHandler.h"
#include <process/Scheduler.h>
#include <panic.h>

PageFaultHandler PageFaultHandler::m_Instance;

#define PAGE_FAULT_EXCEPTION  0x0E
#define PFE_PAGE_PRESENT      0x01
#define PFE_ATTEMPTED_WRITE   0x02
#define PFE_USER_MODE         0x04
#define PFE_RESERVED_BIT      0x08
#define PFE_INSTRUCTION_FETCH 0x10

bool PageFaultHandler::initialise()
{
  InterruptManager &IntManager = InterruptManager::instance();

  return(IntManager.registerInterruptHandler(PAGE_FAULT_EXCEPTION, this));
}

void PageFaultHandler::interrupt(size_t interruptNumber, InterruptState &state)
{
  uint32_t cr2, code;
  asm volatile("mov %%cr2, %%eax" : "=a" (cr2));
  code = state.m_Errorcode;

  //  Get PFE location and error code
  static LargeStaticString sError;
  sError.clear();
  sError.append("Page Fault Exception at 0x");
  sError.append(cr2, 16, 8, '0');
  sError.append(", error code 0x");
  sError.append(code, 16, 8, '0');

  //  Extract error code information
  static LargeStaticString sCode;
  sCode.clear();
  sCode.append("Details: ");

  if(!(code & PFE_PAGE_PRESENT)) sCode.append("NOT ");
  sCode.append("PRESENT | ");

  if(code & PFE_ATTEMPTED_WRITE)
    sCode.append("WRITE | ");
  else
    sCode.append("READ | ");

  if(code & PFE_USER_MODE) sCode.append("USER "); else sCode.append("KERNEL ");
  sCode.append("MODE | ");

  if(code & PFE_RESERVED_BIT) sCode.append("RESERVED BIT SET | ");
  if(code & PFE_INSTRUCTION_FETCH) sCode.append("FETCH |");

  // Ensure the log spinlock isn't going to die on us...
  Log::instance().m_Lock.release();

  ERROR(static_cast<const char*>(sError));
  ERROR(static_cast<const char*>(sCode));

  //static LargeStaticString eCode;
  #ifdef DEBUGGER
    Debugger::instance().start(state, sError);
  #endif

  Scheduler &scheduler = Scheduler::instance();
  if(UNLIKELY(scheduler.getNumProcesses() == 0))
  {
    //  We are in the early stages of the boot process (no processes started)
    panic(sError);
  }
  else
  {
    //  Unrecoverable PFE in a process - Kill the process and yield
    Processor::information().getCurrentThread()->getParent()->kill();

    //  kill member function also calls yield(), so shouldn't get here.
    for(;;) ;
  }

  //  Currently, no code paths return from a PFE.
}

PageFaultHandler::PageFaultHandler()
{
}
