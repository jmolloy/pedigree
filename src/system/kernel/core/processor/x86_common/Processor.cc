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

#include <processor/Processor.h>
#include "PhysicalMemoryManager.h"

void Processor::initialisationDone()
{
  X86CommonPhysicalMemoryManager::instance().initialisationDone();
}

size_t Processor::getDebugBreakpointCount()
{
  return 4;
}

uintptr_t Processor::getDebugBreakpoint(size_t nBpNumber,
                                        DebugFlags::FaultType &nFaultType,
                                        size_t &nLength,
                                        bool &bEnabled)
{
  uintptr_t nLinearAddress = 0;
  switch(nBpNumber)
  {
  case 0:
    asm volatile("mov %%db0, %0" : "=r" (nLinearAddress));
    break;
  case 1:
    asm volatile("mov %%db1, %0" : "=r" (nLinearAddress));
    break;
  case 2:
    asm volatile("mov %%db2, %0" : "=r" (nLinearAddress));
    break;
  case 3:
    asm volatile("mov %%db3, %0" : "=r" (nLinearAddress));
    break;
  }

  uintptr_t nStatus;
  asm volatile("mov %%db7, %0" : "=r" (nStatus));

  bEnabled = static_cast<bool> (nStatus & (1 << (nBpNumber*2+1))); // See intel manual 3b.
  nFaultType = static_cast<DebugFlags::FaultType> ( (nStatus >> (nBpNumber*4+16)) & 0x3 );
  switch ((nStatus >> (nBpNumber*4+18)) & 0x3)
  {
  case 0:
    nLength = 1;
    break;
  case 1:
    nLength = 2;
    break;
  case 2:
    nLength = 8;
    break;
  case 3:
    nLength = 4;
    break;
  }

  return nLinearAddress;
}

void Processor::enableDebugBreakpoint(size_t nBpNumber,
                                      uintptr_t nLinearAddress,
                                      DebugFlags::FaultType nFaultType,
                                      size_t nLength)
{
  switch(nBpNumber)
  {
  case 0:
    asm volatile("mov %0, %%db0" :: "r" (nLinearAddress));
    break;
  case 1:
    asm volatile("mov %0, %%db1" :: "r" (nLinearAddress));
    break;
  case 2:
    asm volatile("mov %0, %%db2" :: "r" (nLinearAddress));
    break;
  case 3:
    asm volatile("mov %0, %%db3" :: "r" (nLinearAddress));
    break;
  }

  uintptr_t nStatus;
  asm volatile("mov %%db7, %0" : "=r" (nStatus));

  size_t lengthField = 0;
  switch (nLength)
  {
  case 1:
    nLength = 0;
    break;
  case 2:
    nLength = 1;
    break;
  case 8:
    nLength = 2;
    break;
  case 4:
    nLength = 3;
    break;
  }

  nStatus |= 1 << (nBpNumber*2+1);
  nStatus |= (nFaultType&0x3) << (nBpNumber*4+16);
  nStatus |= (lengthField&0x3) << (nBpNumber*4+18);
  asm volatile("mov %0, %%db7" :: "r" (nStatus));
}

void Processor::disableDebugBreakpoint(size_t nBpNumber)
{
  uintptr_t nStatus;
  asm volatile("mov %%db7, %0" : "=r" (nStatus));

  nStatus &= ~(1 << (nBpNumber*2+1));
  asm volatile("mov %0, %%db7" :: "r" (nStatus));
}

void Processor::setInterrupts(bool bEnable)
{
  if (bEnable)
    asm volatile("sti");
  else
    asm volatile("cli");
}

void Processor::setSingleStep(bool bEnable, InterruptState &state)
{
  uintptr_t eflags = state.getFlags();
  if (bEnable)
    eflags |= 0x100;
  else
    eflags &= ~0x100;
  state.setFlags(eflags);
}

uint64_t Processor::readMachineSpecificRegister(uint32_t index)
{
  uint32_t eax, edx;
  asm volatile("rdmsr" : "=a" (eax), "=d" (edx) : "c" (index));
  return static_cast<uint64_t>(eax) | (static_cast<uint64_t>(edx) << 32);
}

void Processor::writeMachineSpecificRegister(uint32_t index, uint64_t value)
{
  uint32_t eax = value, edx = value >> 32;
  asm volatile("wrmsr" :: "a" (eax), "d" (edx), "c" (index));
}

void Processor::invalidate(void *pAddress)
{
  asm volatile("invlpg (%0)" :: "a" (pAddress));
}

void Processor::cpuid(uint32_t inEax,
                      uint32_t inEcx,
                      uint32_t &eax,
                      uint32_t &ebx,
                      uint32_t &ecx,
                      uint32_t &edx)
{
    asm volatile("cpuid":
                 "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx):
                 "a" (inEax), "c" (inEcx));
}

#if defined(MULTIPROCESSOR)

  #include "../../../machine/x86_common/Pc.h"
  ProcessorId Processor::id()
  {
    Pc &pc = Pc::instance();
    uint8_t apicId = pc.getLocalApic().getId();

    for (size_t i = 0;i < m_ProcessorInformation.count();i++)
      if (m_ProcessorInformation[i]->m_LocalApicId == apicId)
        return m_ProcessorInformation[i]->m_ProcessorId;

    return 0;
  }
  ProcessorInformation &Processor::information()
  {
    Pc &pc = Pc::instance();
    uint8_t apicId = pc.getLocalApic().getId();

    for (size_t i = 0;i < m_ProcessorInformation.count();i++)
      if (m_ProcessorInformation[i]->m_LocalApicId == apicId)
        return *m_ProcessorInformation[i];

    return *reinterpret_cast<ProcessorInformation*>(0);
  }

#endif
