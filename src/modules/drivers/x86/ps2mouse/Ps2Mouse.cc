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

#include <processor/IoPort.h>

// Defined in src/system/kernel/machine/x86_common/Keyboard.cc
// Can do this as this is an x86-only driver.
IoBase *findPs2(Device *base);

Ps2Mouse::Ps2Mouse(Device *pDev) : m_pBase(0), m_Buffer(), m_BufferIndex(0)
{
    setSpecificType(String("ps2-mouse"));

    /// \todo write me!
}

Ps2Mouse::~Ps2Mouse()
{
}

void Ps2Mouse::initialise(IoBase *pBase)
{
    m_pBase = pBase;
}

bool Ps2Mouse::irq(irq_id_t number, InterruptState &state)
{
    return true;
}

void Ps2Mouse::mouseWait(Ps2Mouse::WaitType type)
{
}

void Ps2Mouse::mouseWrite(uint8_t data)
{
}

uint8_t Ps2Mouse::mouseRead()
{
}

void Ps2Mouse::enableAuxDevice()
{
}

void Ps2Mouse::enableIrq()
{
}

bool Ps2Mouse::setDefaults()
{
    return true;
}

bool Ps2Mouse::enableMouse()
{
    return true;
}
