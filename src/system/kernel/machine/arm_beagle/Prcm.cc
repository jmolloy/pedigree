/*
 * Copyright (c) 2010 Matthew Iselin
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
#include "Prcm.h"
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/Processor.h>
#include <Log.h>

Prcm Prcm::m_Instance;

Prcm::Prcm() : m_Base("PRCM Module")
{
}

Prcm::~Prcm()
{
}

void Prcm::initialise(uintptr_t base)
{
    // Map in the base
    if(!PhysicalMemoryManager::instance().allocateRegion(m_Base,
                                                         2,
                                                         PhysicalMemoryManager::continuous,
                                                         VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode,
                                                         base))
    {
        // Failed to allocate the region!
        return;
    }

    /// \todo Proper initialisation
}

void Prcm::SelectClockPER(size_t clock, Clock which)
{
    if(!m_Base)
    {
        ERROR("PRCM: Not initialised");
        return;
    }

    if(clock == 0) // GPTIMER1 not handled
        return;
    clock -= 1; // GPTIMER2 = 1, starts at bit 0 in the register
    uint32_t bit = 1 << clock;

    // CM_CLKSEL_PER register
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(m_Base.virtualAddress());
    vaddr += PER_CM;
    vaddr += CM_CLKSEL_PER;
    volatile uint32_t *clksel = reinterpret_cast<volatile uint32_t*>(vaddr);
    
    // Set the value if needed
    uint32_t val = *clksel;
    if((which == FCLK_32K) && (val & bit))
        val &= ~bit;
    else if((which == SYS_CLK) && (!(val & bit)))
        val |= bit;
    *clksel = val;
}

void Prcm::SelectClockCORE(size_t clock, Clock which)
{
    if(!m_Base)
    {
        ERROR("PRCM: Not initialised");
        return;
    }

    uint32_t bit = 1 << clock;
    uint32_t set = which << clock;

    uint32_t mask = 0;
    if(clock == 0)
        mask = 0x3;
    else if(clock == 2)
        mask = 0xC;
    else
        mask = bit;

    // CM_CLKSEL_CORE register
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(m_Base.virtualAddress());
    vaddr += CORE_CM;
    vaddr += CM_CLKSEL_CORE;
    volatile uint32_t *clksel = reinterpret_cast<volatile uint32_t*>(vaddr);
    
    // Set the value if needed
    uint32_t val = *clksel;
    val &= ~mask;
    val |= set;
    *clksel = val;
}

void Prcm::SetFuncClockPER(size_t clock, bool enabled)
{
    if(!m_Base)
    {
        ERROR("PRCM: Not initialised");
        return;
    }

    if(clock == 0) // GPTIMER1 not handled
        return;
    clock += 2; // GPTIMER2 = 1, starts at bit 3 in the register
    uint32_t bit = 1 << clock;

    // CM_CLKSEL_PER register
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(m_Base.virtualAddress());
    vaddr += PER_CM;
    vaddr += CM_FCLKEN_PER;
    volatile uint32_t *clksel = reinterpret_cast<volatile uint32_t*>(vaddr);
    
    // Set the value if needed
    uint32_t val = *clksel;
    if((!enabled) && (val & bit))
        val &= ~bit;
    else if(enabled && (!(val & bit)))
        val |= bit;
    *clksel = val;
}

void Prcm::SetIfaceClockPER(size_t clock, bool enabled)
{
    if(!m_Base)
    {
        ERROR("PRCM: Not initialised");
        return;
    }

    if(clock == 0) // GPTIMER1 not handled
        return;
    clock += 2; // GPTIMER2 = 1, starts at bit 3 in the register
    uint32_t bit = 1 << clock;

    // CM_CLKSEL_PER register
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(m_Base.virtualAddress());
    vaddr += PER_CM;
    vaddr += CM_ICLKEN_PER;
    volatile uint32_t *clksel = reinterpret_cast<volatile uint32_t*>(vaddr);
    
    // Set the value if needed
    uint32_t val = *clksel;
    if((!enabled) && (val & bit))
        val &= ~bit;
    else if(enabled && (!(val & bit)))
        val |= bit;
    *clksel = val;
}

void Prcm::SetFuncClockCORE(size_t n, size_t clock, bool enabled)
{
    if(!m_Base)
    {
        ERROR("PRCM: Not initialised");
        return;
    }

    // Bit to set
    uint32_t bit = 1 << clock;

    // CM_FCLKENn_CORE register
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(m_Base.virtualAddress());
    vaddr += CORE_CM;
    if(n == 1)
        vaddr += CM_FCLKEN1_CORE;
    else if(n == 3)
        vaddr += CM_FCLKEN3_CORE;
    else
    {
        WARNING("PRCM: Invalid functional clock enable bank (CORE domain)");
        return;
    }
    volatile uint32_t *clksel = reinterpret_cast<volatile uint32_t*>(vaddr);
    
    // Set the value if needed
    uint32_t val = *clksel;
    if((!enabled) && (val & bit))
        val &= ~bit;
    else if(enabled && (!(val & bit)))
        val |= bit;
    *clksel = val;
}

void Prcm::SetIfaceClockCORE(size_t n, size_t clock, bool enabled)
{
    if(!m_Base)
    {
        ERROR("PRCM: Not initialised");
        return;
    }

    // Bit to set
    uint32_t bit = 1 << clock;

    // CM_ICLKENn_CORE register
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(m_Base.virtualAddress());
    vaddr += CORE_CM;
    if(n == 1)
        vaddr += CM_ICLKEN1_CORE;
    else if(n == 3)
        vaddr += CM_ICLKEN3_CORE;
    else
    {
        WARNING("PRCM: Invalid interface clock enable bank (CORE domain)");
        return;
    }
    volatile uint32_t *clksel = reinterpret_cast<volatile uint32_t*>(vaddr);
    
    // Set the value if needed
    uint32_t val = *clksel;
    if((!enabled) && (val & bit))
        val &= ~bit;
    else if(enabled && (!(val & bit)))
        val |= bit;
    *clksel = val;
}

void Prcm::WaitCoreIdleStatus(size_t n, size_t clock, bool waitForOn)
{
    if(!m_Base)
    {
        ERROR("PRCM: Not initialised");
        return;
    }

    // Bit to set
    uint32_t bit = 1 << clock;

    // CM_IDLESTn_CORE register
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(m_Base.virtualAddress());
    vaddr += CORE_CM;
    if(n == 1)
        vaddr += CM_IDLEST1_CORE;
    else if(n == 3)
        vaddr += CM_IDLEST3_CORE;
    else
    {
        WARNING("PRCM: Invalid idle status bank (CORE domain)");
        return;
    }
    volatile uint32_t *clksel = reinterpret_cast<volatile uint32_t*>(vaddr);

    // When the bit transitions to zero, the module is accessible
    /// \todo delays or something
    if(waitForOn)
        while(*clksel & bit);
    else
        while(!(*clksel & bit));
}

void Prcm::WaitPllIdleStatus(size_t n, size_t clock, bool waitForOn)
{
    if(!m_Base)
    {
        ERROR("PRCM: Not initialised");
        return;
    }

    // Bit to set
    uint32_t bit = 1 << clock;

    // CM_IDLESTn_PLL register
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(m_Base.virtualAddress());
    vaddr += Clock_Control_Reg_CM;
    if(n == 1)
        vaddr += CM_IDLEST_CKGEN;
    else if(n == 2)
        vaddr += CM_IDLEST2_CKGEN;
    else
    {
        WARNING("PRCM: Invalid idle status bank (CORE domain)");
        return;
    }
    volatile uint32_t *clksel = reinterpret_cast<volatile uint32_t*>(vaddr);

    // When the bit transitions to one, the clock is locked
    /// \todo delays or something
    if(waitForOn)
        while(!(*clksel & bit));
    else
        while(*clksel & bit);
}

void Prcm::SelectClockPLL(size_t n, size_t value)
{
    if(!m_Base)
    {
        ERROR("PRCM: Not initialised");
        return;
    }

    // CM_CLKSELn_PLL register
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(m_Base.virtualAddress());
    vaddr += Clock_Control_Reg_CM;
    if(n == 1)
        vaddr += CM_CLKSEL1_PLL;
    else if(n == 2)
        vaddr += CM_CLKSEL2_PLL;
    else if(n == 3)
        vaddr += CM_CLKSEL3_PLL;
    else if(n == 4)
        vaddr += CM_CLKSEL4_PLL;
    else if(n == 5)
        vaddr += CM_CLKSEL5_PLL;
    volatile uint32_t *clksel = reinterpret_cast<volatile uint32_t*>(vaddr);
    
    // Set it!
    *clksel = value;
}

void Prcm::SetClockPLL(size_t n, size_t value)
{
    if(!m_Base)
    {
        ERROR("PRCM: Not initialised");
        return;
    }

    // CM_CLKENn_PLL register
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(m_Base.virtualAddress());
    vaddr += Clock_Control_Reg_CM;
    if(n == 1)
        vaddr += CM_CLKEN_PLL;
    else if(n == 2)
        vaddr += CM_CLKEN2_PLL;
    else
    {
        WARNING("PRCM: Invalid interface clock enable bank (CORE domain)");
        return;
    }
    volatile uint32_t *clksel = reinterpret_cast<volatile uint32_t*>(vaddr);
    
    // Set it!
    *clksel = value;
}
