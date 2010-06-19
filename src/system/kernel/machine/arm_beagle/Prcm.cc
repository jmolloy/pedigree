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
        return;

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
        val ^= bit;
    else if((which == SYS_CLK) && (!(val & bit)))
        val |= bit;
    *clksel = val;
}

void Prcm::SetFuncClockPER(size_t clock, bool enabled)
{
    if(!m_Base)
        return;

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
        val ^= bit;
    else if(enabled && (!(val & bit)))
        val |= bit;
    *clksel = val;
}

void Prcm::SetIfaceClockPER(size_t clock, bool enabled)
{
    if(!m_Base)
        return;

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
        val ^= bit;
    else if(enabled && (!(val & bit)))
        val |= bit;
    *clksel = val;
}
