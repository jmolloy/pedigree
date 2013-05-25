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

#include "Ps2Mouse.h"

#include <processor/IoBase.h>
#include <machine/Machine.h>
#include <processor/Processor.h>
#include <machine/IrqManager.h>
#include <machine/InputManager.h>
#include <LockGuard.h>
#include <Log.h>

Ps2Mouse::Ps2Mouse(Device *pDev) : m_pBase(0), m_Buffer(), m_BufferIndex(0), m_BufferLock(), m_IrqWait(0)
{
    setSpecificType(String("ps2-mouse"));

    // Install ourselves as the IRQ handler for the mouse
    setInterruptNumber(12);
    Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*>(this));
}

Ps2Mouse::~Ps2Mouse()
{
}

bool Ps2Mouse::initialise(IoBase *pBase)
{
    m_pBase = pBase;

    // Enable the auxillary PS/2 device
    enableAuxDevice();

    // Enable the mouse IRQ
    enableIrq();

    // Set default settings on the mouse
    if(!setDefaults())
        return false;

    // Finally, enable the mouse
    if(!enableMouse())
        return false;

    return true;
}

bool Ps2Mouse::irq(irq_id_t number, InterruptState &state)
{
    uint8_t b = m_pBase->read8(4);

    if((b & 0x01) && (b & 0x20))
    {
        b = m_pBase->read8();
        if(b == MouseAck)
        {
            m_IrqWait.release();
        }
        else
        {
            LockGuard<Spinlock> guard(m_BufferLock);

            m_Buffer[m_BufferIndex++] = b;
            if(m_BufferIndex == 3)
            {
                InputManager::instance().mouseUpdate(m_Buffer[1], m_Buffer[2], m_Buffer[0] & 0x3);
                m_BufferIndex = 0;
            }
        }
    }

    return true;
}

void Ps2Mouse::mouseWait(Ps2Mouse::WaitType type)
{
    // Pretend to wait.
    /// \todo Check for data
}

void Ps2Mouse::mouseWrite(uint8_t data)
{
    // Wait for the controller
    mouseWait(Signal);

    // Switch to the mouse
    m_pBase->write8(0xD4, 4);

    // Wait for it to be selected
    mouseWait(Signal);

    // Send the byte
    m_pBase->write8(data, 0);
}

uint8_t Ps2Mouse::mouseRead()
{
    // Wait for the mouse to have data for us
    mouseWait(Data);

    // Read the data byte
    return m_pBase->read8(0);
}

void Ps2Mouse::enableAuxDevice()
{
    // Wait for the controller
    mouseWait(Signal);

    // Enable the auxillary device
    m_pBase->write8(0xA8, 4);
}

void Ps2Mouse::enableIrq()
{
    // Wait for the controller
    mouseWait(Signal);

    // Enable the mouse interrupt
    m_pBase->write8(0x20, 4);
    mouseWait(Data);
    uint8_t status = m_pBase->read8(0) | 2;
    mouseWait(Signal);
    m_pBase->write8(0x60, 4);
    mouseWait(Signal);
    m_pBase->write8(status, 0);
}

bool Ps2Mouse::setDefaults()
{
    // Wait for the controller
    mouseWait(Signal);

    // Tell the mouse to set defaults
    mouseWrite(SetDefaults);

    m_IrqWait.acquire(1, 0, 500);
    if(Processor::information().getCurrentThread()->wasInterrupted())
    {
        // Read the acknowledgement byte
        uint8_t ack = mouseRead();
        if(ack != MouseAck)
        {
            NOTICE("Ps2Mouse: mouse didn't ack SetDefaults");
            return false;
        }
    }

    return true;
}

bool Ps2Mouse::enableMouse()
{
    // Wait for the controller
    mouseWait(Signal);

    // Enable the mouse
    mouseWrite(0xF4);

    m_IrqWait.acquire(1, 0, 500);
    if(Processor::information().getCurrentThread()->wasInterrupted())
    {
        // Read the acknowledgement byte
        uint8_t ack = mouseRead();
        if(ack != MouseAck)
        {
            NOTICE("Ps2Mouse: mouse didn't ack Enable");
            return false;
        }
    }

    return true;
}
