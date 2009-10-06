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

#include <Log.h>
#include <processor/types.h>
#include <process/Event.h>
#include <process/SignalEvent.h>

SignalEvent::SignalEvent(uintptr_t handlerAddress, size_t signalNum, size_t specificNestingLevel) :
    Event(handlerAddress, false, specificNestingLevel), m_SignalNumber(signalNum)
{
}

SignalEvent::~SignalEvent()
{
}

/// \todo There may be a need for serialization in the future...
size_t SignalEvent::serialize(uint8_t *pBuffer)
{
    pBuffer[0] = static_cast<uint8_t>(m_SignalNumber & 0xFF);
    return 1;
}

bool SignalEvent::unserialize(uint8_t *pBuffer, Event &event)
{
    pBuffer[0] = static_cast<uint8_t>(event.getNumber() & 0xFF);
    return true;
}
