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
#include <Log.h>

size_t Processor::getDebugBreakpointCount()
{
  return 1;
}

uintptr_t Processor::getDebugBreakpoint(size_t nBpNumber,
                                        DebugFlags::FaultType &nFaultType,
                                        size_t &nLength,
                                        bool &bEnabled)
{
  if (nBpNumber > 0)
  {
    ERROR("Breakpoint out of bounds.");
    return 0;
  }

  // Get the register.
  uint32_t watchLo;
  asm volatile("mfc0 %0, $18; nop" : "=r" (watchLo));
  
  switch (watchLo & 0x3)
  {
    case 0:
      bEnabled = false;
      nFaultType = DebugFlags::DataReadWrite;
      break;
    case 1:
      bEnabled = true;
      nFaultType = DebugFlags::DataWrite;
      break;
    case 3:
      bEnabled = true;
      nFaultType = DebugFlags::DataReadWrite;
      break;
    default:
      bEnabled = false;
      nFaultType = DebugFlags::DataReadWrite;
  }
  nLength = 8;

  return watchLo & 0xFFFFFFFC;
}

void Processor::enableDebugBreakpoint(size_t nBpNumber,
                                      uintptr_t nLinearAddress,
                                      DebugFlags::FaultType nFaultType,
                                      size_t nLength)
{
  if (nBpNumber > 0)
  {
    ERROR("Breakpoint out of bounds.");
    return;
  }

  // Convert to physical address.
  nLinearAddress &= ~0xc0000000;

  if (nFaultType == DebugFlags::DataWrite) nLinearAddress |= 0x1;
  else nLinearAddress |= 0x3;

  asm volatile("mtc0 %0, $18; nop" : : "r" (nLinearAddress));
}

void Processor::disableDebugBreakpoint(size_t nBpNumber)
{
  if (nBpNumber > 0)
  {
    ERROR("Breakpoint out of bounds.");
    return;
  }

  uint32_t watchLo = 0;
  asm volatile("mtc0 %0, $18; nop" : : "r" (watchLo));
}

void Processor::setInterrupts(bool bEnable)
{
  uint32_t sr;
  asm volatile("mfc0 %0, $12;nop" : "=r" (sr));
  if (bEnable)
    sr |= SR_IE;
  else
    sr &= ~SR_IE;
  asm volatile("mtc0 %0, $12;nop" : : "r" (sr));
}

void Processor::setSingleStep(bool bEnable, InterruptState &state)
{
  /// \todo Implement - MIPS doesn't have a single step mechanism per se...
  ERROR("Single step unavailable on MIPS.");
}

void Processor::invalidateICache(uintptr_t nAddr)
{
  asm volatile("cache 0x10, 0(%0)" : : "r"(nAddr));
}

void Processor::invalidateDCache(uintptr_t nAddr)
{
  asm volatile("cache 0x11, 0(%0)" : : "r"(nAddr));
}
