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
#include "InterruptManager.h"
#include "PhysicalMemoryManager.h"

void Processor::initialise1(const BootstrapStruct_t &Info)
{
    // Initialise this processor's interrupt handling
    // ARMV7InterruptManager::initialiseProcessor();

    // TODO: Initialise the physical memory-management
    Arm7PhysicalMemoryManager::instance().initialise(Info);

    // TODO

    m_Initialised = 1;
}

void Processor::initialise2(const BootstrapStruct_t &Info)
{
  // TODO

//   m_Initialised = 2;
}

void Processor::identify(HugeStaticString &str)
{
    // Read the Main ID register
    union
    {
        uint32_t data;

        struct {
            uint32_t revision : 4;
            uint32_t partnum : 10;
            uint32_t arch : 4;
            uint32_t variant : 4;
            char implementer;
        } PACKED;
    } mainID;
    asm volatile("mrc p15, 0, %0, c0, c0, 0" : "=r" (mainID.data));

    // Grab the implementer of this chip
    switch(mainID.implementer)
    {
        case 'A':
            str += "ARM ";
            break;
        case 'D':
            str += "DEC ";
            break;
        case 'M':
            str += "Motorola/Freescale ";
            break;
        case 'Q':
            str += "Qualcomm ";
            break;
        case 'V':
            str += "Marvell ";
            break;
        case 'i':
            str += "Intel ";
            break;
        default:
            str += "Unknown ";
            break;
    }

    // Grab the architecture
    switch(mainID.arch)
    {
        case 0x1:
            str += "ARMv4 ";
            break;
        case 0x2:
            str += "ARMv4T ";
            break;
        case 0x3:
            str += "ARMv5 ";
            break;
        case 0x4:
            str += "ARMv5T ";
            break;
        case 0x5:
            str += "ARMv5TE ";
            break;
        case 0x6:
            str += "ARMv5TEJ ";
            break;
        case 0x7:
            str += "ARMv6 ";
            break;
        case 0xF:
            str += "ARMv7 or above ";
            break;
        default:
            str += "(unknown architecture) ";
            break;
    }

    // Append the part number
    str += "(part number: ";
    str.append(mainID.partnum);
    str += ", revision maj=";

    // Append the revision
    str.append(mainID.variant);
    str += " min=";
    str.append(mainID.revision);
    str += ")";
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

bool Processor::getInterrupts()
{
  return false;
}

void Processor::setSingleStep(bool bEnable, InterruptState &state)
{
  /// \todo Implement
  ERROR("Single step unavailable on ARM.");
}

void Processor::switchAddressSpace(VirtualAddressSpace &AddressSpace)
{
  ERROR("ARM has no address space support yet");
}
