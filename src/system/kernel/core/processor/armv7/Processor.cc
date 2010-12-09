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
#include "VirtualAddressSpace.h"
#include <process/initialiseMultitasking.h>

void Processor::initialise1(const BootstrapStruct_t &Info)
{
    // Initialise the physical memory-management
    ArmV7PhysicalMemoryManager::instance().initialise(Info);

    // Initialise this processor's interrupt handling
    ARMV7InterruptManager::initialiseProcessor();

    // Map in the base ELF file we loaded from
    /// \todo Unmap.
    VirtualAddressSpace &va = VirtualAddressSpace::getKernelAddressSpace();
    
    uintptr_t mapBase   = Info.mods_addr;
    size_t mapLen       = Info.mods_count;

    for(size_t i = 0; i < mapLen; i+= 0x1000)
    {
        va.map(mapBase + i, reinterpret_cast<void*>(mapBase + i), VirtualAddressSpace::KernelMode);
    }

    m_Initialised = 1;
}

void Processor::initialise2(const BootstrapStruct_t &Info)
{
    // Initialise multitasking
#ifdef THREADS
    initialiseMultitasking();
#endif

    m_Initialised = 2;
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
    bool bCurrent = getInterrupts();
    if(bCurrent == bEnable)
        return;

    /// \todo FIQs count too
    if(m_Initialised >= 1) // Interrupts are only initialised at phase 1
    {
        uint32_t cpsr = 0;
        asm volatile("MRS %0, cpsr" : "=r" (cpsr));
        if(bEnable)
            cpsr ^= 0x80;
        else
            cpsr |= 0x80;
        asm volatile("MSR cpsr_c, %0" : : "r" (cpsr));
    }
}

bool Processor::getInterrupts()
{
    /// \todo FIQs count too
    if(m_Initialised >= 1) // Interrupts are only initialised at phase 1
    {
        uint32_t cpsr = 0;
        asm volatile("MRS %0, cpsr" : "=r" (cpsr));
        return !(cpsr & 0x80);
    }
    else
        return false;
}

void Processor::setSingleStep(bool bEnable, InterruptState &state)
{
  /// \todo Implement
  ERROR("Single step unavailable on ARM.");
}

void Processor::switchAddressSpace(VirtualAddressSpace &AddressSpace)
{
    const ArmV7VirtualAddressSpace &armAddressSpace = static_cast<const ArmV7VirtualAddressSpace&>(AddressSpace);

    // Do we need to set a new page directory?
    if (readTTBR0() != armAddressSpace.m_PhysicalPageDirectory)
    {
        // Set the new page directory
        writeTTBR0(armAddressSpace.m_PhysicalPageDirectory);

        // Update the information in the ProcessorInformation structure
        ProcessorInformation &processorInformation = Processor::information();
        processorInformation.setVirtualAddressSpace(AddressSpace);
    }
}

physical_uintptr_t Processor::readTTBR0()
{
    physical_uintptr_t ret = 0;
    asm volatile("MRC p15,0,%0,c2,c0,0" : "=r" (ret));
    return ret;
}
physical_uintptr_t Processor::readTTBR1()
{
    physical_uintptr_t ret = 0;
    asm volatile("MRC p15,0,%0,c2,c0,1" : "=r" (ret));
    return ret;
}
physical_uintptr_t Processor::readTTBCR()
{
    physical_uintptr_t ret = 0;
    asm volatile("MRC p15,0,%0,c2,c0,2" : "=r" (ret));
    return ret;
}

void Processor::writeTTBR0(physical_uintptr_t value)
{
    asm volatile("MCR p15,0,%0,c2,c0,0" : : "r" (value));
}
void Processor::writeTTBR1(physical_uintptr_t value)
{
    asm volatile("MCR p15,0,%0,c2,c0,1" : : "r" (value));
}
void Processor::writeTTBCR(uint32_t value)
{
    asm volatile("MCR p15,0,%0,c2,c0,2" : : "r" (value));
}

void Processor::setTlsBase(uintptr_t newBase)
{
    // Using the user read-only thread and process ID register to store the TLS base
    asm volatile("MCR p15,0,%0,c13,c0,3" : : "r" (newBase));
}

