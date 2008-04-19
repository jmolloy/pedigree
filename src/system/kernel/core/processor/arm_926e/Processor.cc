/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#include "InterruptManager.h"

void Processor::initialise1(const BootstrapStruct_t &Info)
{
  // Initialise this processor's interrupt handling
   //MIPS32InterruptManager::initialiseProcessor();

  // TODO: Initialise the physical memory-management

  // TODO

//   m_Initialised = 1;
}

void Processor::initialise2()
{
  // TODO

//   m_Initialised = 2;
}

void Processor::identify(HugeStaticString &str)
{
  // TODO: this is kinda boring, i want inline assembly
  str += "Arm926E";
}

size_t Processor::getDebugBreakpointCount()
{
  return 0;
}

uintptr_t Processor::getDebugBreakpoint(size_t nBpNumber,
                                        DebugFlags::FaultType &nFaultType,
                                        size_t &nLength,
                                        bool &bEnabled)
{
  /// \todo Implement.
  return 0;
}

void Processor::enableDebugBreakpoint(size_t nBpNumber,
                                      uintptr_t nLinearAddress,
                                      DebugFlags::FaultType nFaultType,
                                      size_t nLength)
{
  /// \todo Implement.
}

void Processor::disableDebugBreakpoint(size_t nBpNumber)
{
  /// \todo Implement.
}

void Processor::setInterrupts(bool bEnable)
{
  /// \todo Implement.
}

void Processor::setSingleStep(bool bEnable, InterruptState &state)
{
  /// \todo Implement - should be as simple
  ERROR("Single step unavailable on ARM.");
}
