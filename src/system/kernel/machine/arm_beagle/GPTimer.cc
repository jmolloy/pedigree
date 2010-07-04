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
#include "GPTimer.h"
#include "Prcm.h"
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/Processor.h>
#include <Log.h>

#include <machine/Machine.h>

SyncTimer g_SyncTimer;

void SyncTimer::initialise(uintptr_t base)
{
    // Map in the base
    if(!PhysicalMemoryManager::instance().allocateRegion(m_MmioBase,
                                                         1,
                                                         PhysicalMemoryManager::continuous,
                                                         VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode,
                                                         base))
    {
        // Failed to allocate the region!
        return;
    }

    // Dump the hardware revision
    uint32_t hardwareRevision = *reinterpret_cast<volatile uint32_t*>(m_MmioBase.virtualAddress());
    NOTICE("32-kHz sync timer at " <<
           Hex << reinterpret_cast<uintptr_t>(m_MmioBase.virtualAddress()) <<
           "  - revision " << Dec <<
           ((hardwareRevision >> 4) & 0xF) << "." <<
           (hardwareRevision & 0xF) << Hex);
}

uint32_t SyncTimer::getTickCount()
{
    if(!m_MmioBase)
        return 0;

    return reinterpret_cast<uint32_t*>(m_MmioBase.virtualAddress())[4];
}

void GPTimer::initialise(size_t timer, uintptr_t base)
{
    // Map in the base
    if(!PhysicalMemoryManager::instance().allocateRegion(m_MmioBase,
                                                         1,
                                                         PhysicalMemoryManager::continuous,
                                                         VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode,
                                                         base))
    {
        // Failed to allocate the region!
        return;
    }

    if(timer > 0)
    {
        // Use the SYS_CLK as our time source during reset
        Prcm::instance().SelectClockPER(timer, Prcm::SYS_CLK);

        // Enable the interface clock for the timer
        Prcm::instance().SetIfaceClockPER(timer, true);

        // Enable the functional clock for the timer
        Prcm::instance().SetFuncClockPER(timer, true);
    }

    volatile uint32_t *registers = reinterpret_cast<volatile uint32_t*>(m_MmioBase.virtualAddress());

    // Reset the timer
    registers[TIOCP_CFG] = 2;
    registers[TSICR] = 2;
    while(!(registers[TISTAT] & 1));

    if(timer > 0)
    {
        // Reset the clock source to the 32 kHz functional clock
        Prcm::instance().SelectClockPER(timer, Prcm::FCLK_32K);
    }

    // Dump the hardware revision
    uint32_t hardwareRevision = registers[TIDR];
    NOTICE("General Purpose timer at " <<
           Hex << reinterpret_cast<uintptr_t>(m_MmioBase.virtualAddress()) <<
           "  - revision " << Dec <<
           ((hardwareRevision >> 4) & 0xF) << "." <<
           (hardwareRevision & 0xF) << Hex);

    // Section 16.2.4.2.1 in the OMAP35xx manual, page 2573
    registers[TPIR] = 232000;
    registers[TNIR] = -768000;
    registers[TLDR] = 0xFFFFFFE0;
    registers[TTGR] = 1; // Trigger after one interval

    // Clear existing interrupts
    registers[TISR] = 7;

    // Set the IRQ number for future reference
    m_Irq = 37 + timer;

    // Enable the overflow interrupt
    registers[TIER] = 2;

    // Enable the timer in the right mode
    registers[TCLR] = 3; // Autoreload and timer started
}

bool GPTimer::registerHandler(TimerHandler *handler)
{
    // Find a spare spot and install
    size_t nHandler;
    for(nHandler = 0; nHandler < MAX_TIMER_HANDLERS; nHandler++)
    {
        if(m_Handlers[nHandler] == 0)
        {
            m_Handlers[nHandler] = handler;

            if(!m_bIrqInstalled && m_Irq)
            {
                Spinlock install;
                install.acquire();
                m_bIrqInstalled = InterruptManager::instance().registerInterruptHandler(m_Irq, this);
                install.release();
            }

            return true;
        }
    }

    // No room!
    return false;
}

bool GPTimer::unregisterHandler(TimerHandler *handler)
{
    size_t nHandler;
    for(nHandler = 0; nHandler < MAX_TIMER_HANDLERS; nHandler++)
    {
        if(m_Handlers[nHandler] == handler)
        {
            m_Handlers[nHandler] = 0;
            return true;
        }
    }
    return false;
}

void GPTimer::addAlarm(class Event *pEvent, size_t alarmSecs, size_t alarmUsecs)
{
#ifdef THREADS
    size_t time = (alarmSecs * 1000) + (alarmUsecs / 1000) + m_TickCount;
    Alarm *pAlarm = new Alarm(pEvent, time,
                              Processor::information().getCurrentThread());
    m_Alarms.pushBack(pAlarm);
#endif
}

void GPTimer::removeAlarm(class Event *pEvent)
{
#ifdef THREADS
    for (List<Alarm*>::Iterator it = m_Alarms.begin();
         it != m_Alarms.end();
         it++)
    {
        if ( (*it)->m_pEvent == pEvent )
        {
            m_Alarms.erase(it);
            return;
        }
    }
#endif
}

size_t GPTimer::removeAlarm(class Event *pEvent, bool bRetZero)
{
#ifdef THREADS
    size_t currTime = getTickCount();

    for (List<Alarm*>::Iterator it = m_Alarms.begin();
         it != m_Alarms.end();
         it++)
    {
        if ( (*it)->m_pEvent == pEvent )
        {
            size_t ret = 0;
            if(!bRetZero)
            {
                size_t alarmEndTime = (*it)->m_Time;

                // Is it later than the end of the alarm?
                if(alarmEndTime < currTime)
                    ret = 0;
                else
                {
                    size_t diff = alarmEndTime - currTime;
                    ret = (diff / 1000) + 1;
                }
            }

            m_Alarms.erase(it);
            return ret;
        }
    }

    return 0;
#else
    return 0;
#endif
}

void GPTimer::interrupt(size_t nInterruptnumber, InterruptState &state)
{
    m_TickCount++;

    // Call handlers
    size_t nHandler;
    for(nHandler = 0; nHandler < MAX_TIMER_HANDLERS; nHandler++)
    {
        // Timer delta is in nanoseconds
        if(m_Handlers[nHandler])
            m_Handlers[nHandler]->timer(1000000, state);
    }

    // Check for alarms.
    while (true)
    {
        bool bDispatched = false;
        for (List<Alarm*>::Iterator it = m_Alarms.begin();
        it != m_Alarms.end();
        it++)
        {
            Alarm *pA = *it;
            if ( pA->m_Time <= m_TickCount )
            {
                pA->m_pThread->sendEvent(pA->m_pEvent);
                m_Alarms.erase(it);
                bDispatched = true;
                break;
            }
        }
        if (!bDispatched)
            break;
    }

    // Ack the interrupt source
    volatile uint32_t *registers = reinterpret_cast<volatile uint32_t*>(m_MmioBase.virtualAddress());
    registers[TISR] = registers[TISR];
}
