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

#include "InterruptManager.h"
#include <machine/Machine.h>
#include <machine/types.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <Debugger.h>
#include <Log.h>
#include <machine/openfirmware/OpenFirmware.h>
#include <machine/openfirmware/Device.h>

extern "C" int isr_reset;
extern "C" int isr_machine_check;
extern "C" int isr_dsi;
extern "C" int isr_isi;
extern "C" int isr_interrupt;
extern "C" int isr_alignment;
extern "C" int isr_program;
extern "C" int isr_fpu;
extern "C" int isr_decrementer;
extern "C" int isr_sc;
extern "C" int isr_trace;
extern "C" int isr_perf_mon;
extern "C" int isr_instr_breakpoint;
extern "C" int isr_system_management;
extern "C" int isr_thermal_management;

PPC32InterruptManager PPC32InterruptManager::m_Instance;

SyscallManager &SyscallManager::instance()
{
  return PPC32InterruptManager::instance();
}
InterruptManager &InterruptManager::instance()
{
  return PPC32InterruptManager::instance();
}

bool PPC32InterruptManager::registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler)
{
  return true;
}

#ifdef DEBUGGER
  bool PPC32InterruptManager::registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler)
  {
    return true;
  }
  size_t PPC32InterruptManager::getBreakpointInterruptNumber()
  {
    return 3;
  }
  size_t PPC32InterruptManager::getDebugInterruptNumber()
  {
    return 1;
  }
#endif

bool PPC32InterruptManager::registerSyscallHandler(Service_t Service, SyscallHandler *handler)
{
  return true;
}

void PPC32InterruptManager::initialiseProcessor()
{
  // We know that we get called before the virtual address space is initialised, so
  // we'll have to do the identity mapping ourselves. How crude!
  OFDevice chosen (OpenFirmware::instance().findDevice("/chosen"));
  OFDevice mmu (chosen.getProperty("mmu"));

  // Identity map the lower area of memory.
  /*  OFParam ret = mmu.executeMethod("map", 4,
                    reinterpret_cast<OFParam>(-1),
                    reinterpret_cast<OFParam>(0x3000),
                    reinterpret_cast<OFParam>(0x0),
                    reinterpret_cast<OFParam>(0x0));
  */
  // Copy the interrupt handlers into lower memory.
  memcpy(reinterpret_cast<void*> (0x0100), &isr_reset, 0x100);
  memcpy(reinterpret_cast<void*> (0x0200), &isr_machine_check, 0x100);
  memcpy(reinterpret_cast<void*> (0x0300), &isr_dsi, 0x100);
  memcpy(reinterpret_cast<void*> (0x0400), &isr_isi, 0x100);
  memcpy(reinterpret_cast<void*> (0x0500), &isr_interrupt, 0x100);
  memcpy(reinterpret_cast<void*> (0x0600), &isr_alignment, 0x100);
  memcpy(reinterpret_cast<void*> (0x0700), &isr_program, 0x100);
  memcpy(reinterpret_cast<void*> (0x0800), &isr_fpu, 0x100);
  memcpy(reinterpret_cast<void*> (0x0900), &isr_decrementer, 0x100);
  memcpy(reinterpret_cast<void*> (0x0C00), &isr_sc, 0x100);
  memcpy(reinterpret_cast<void*> (0x0D00), &isr_trace, 0x100);
  memcpy(reinterpret_cast<void*> (0x0F00), &isr_perf_mon, 0x100);
  memcpy(reinterpret_cast<void*> (0x1300), &isr_instr_breakpoint, 0x100);
  memcpy(reinterpret_cast<void*> (0x1400), &isr_system_management, 0x100);
  memcpy(reinterpret_cast<void*> (0x1700), &isr_thermal_management, 0x100);

  for (uintptr_t i = 0x0; i < 0x1800; i += 4)
    asm volatile("dcbst 0, %0" : : "r" (i));

  asm volatile("sync");

  for (uintptr_t i = 0; i < 0x1800; i += 4)
    Processor::invalidateICache(i);
  
  asm volatile("sync");
  asm volatile("isync");
}

void PPC32InterruptManager::interrupt(InterruptState &interruptState)
{
  static LargeStaticString msg("Interrupt!");
  Debugger::instance().start(interruptState, msg);
  for(;;);
}

PPC32InterruptManager::PPC32InterruptManager()
{
}
PPC32InterruptManager::~PPC32InterruptManager()
{
}
