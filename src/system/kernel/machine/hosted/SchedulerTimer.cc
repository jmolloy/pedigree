/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include <compiler.h>
#include <machine/Machine.h>
#include <Log.h>
#include "SchedulerTimer.h"

using namespace __pedigree_hosted;

/** 10 hertz frequency. */
#define ONE_SECOND 1000000000
#define HZ 10

HostedSchedulerTimer HostedSchedulerTimer::m_Instance;

bool HostedSchedulerTimer::registerHandler(TimerHandler *handler)
{
    if (UNLIKELY(handler == 0 && m_Handler != 0))
        return false;

    m_Handler = handler;
    return true;
}

bool HostedSchedulerTimer::initialise()
{
    struct sigevent sv;
    ByteSet(&sv, 0, sizeof(sv));
    sv.sigev_notify = SIGEV_SIGNAL;
    sv.sigev_signo = SIGUSR2;
    sv.sigev_value.sival_ptr = this;
    int r = timer_create(CLOCK_REALTIME, &sv, &m_Timer);
    if(r != 0)
    {
        /// \todo error message or something
        return false;
    }

    struct itimerspec interval;
    ByteSet(&interval, 0, sizeof(interval));
    interval.it_interval.tv_nsec = ONE_SECOND / HZ;
    interval.it_value.tv_nsec = ONE_SECOND / HZ;
    r = timer_settime(m_Timer, 0, &interval, 0);
    if(r != 0)
    {
        timer_delete(m_Timer);
        return false;
    }

    IrqManager &irqManager = *Machine::instance().getIrqManager();
    m_IrqId = irqManager.registerIsaIrqHandler(1, this);
    if (m_IrqId == 0)
    {
        timer_delete(m_Timer);
        return false;
    }

    return true;
}
void HostedSchedulerTimer::uninitialise()
{
    timer_delete(m_Timer);

    // Free the IRQ
    if (m_IrqId != 0)
    {
        IrqManager &irqManager = *Machine::instance().getIrqManager();
        irqManager.unregisterHandler(m_IrqId, this);
    }
}

HostedSchedulerTimer::HostedSchedulerTimer() : m_IrqId(0), m_Handler(0)
{
}

bool HostedSchedulerTimer::irq(irq_id_t number, InterruptState &state)
{
    // Should we handle this?
    uint64_t opaque = state.getRegister(0);
    if(opaque != reinterpret_cast<uint64_t>(this))
    {
        return false;
    }

    // TODO: Delta is wrong
    if(LIKELY(m_Handler != 0))
        m_Handler->timer(ONE_SECOND / HZ, state);

    return true;
}
