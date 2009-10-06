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
#ifndef SIGNAL_EVENT_H
#define SIGNAL_EVENT_H

#include <process/Event.h>

class SignalEvent : public Event
{
public:
    SignalEvent(uintptr_t handlerAddress, size_t signalNum, size_t specificNestingLevel=~0UL);
    virtual ~SignalEvent();

    virtual size_t serialize(uint8_t *pBuffer);
    static bool unserialize(uint8_t *pBuffer, Event &event);

    virtual size_t getNumber()
    {
        return m_SignalNumber;
    }

private:
    /** This keeps track of the actual signal this SignalEvent is linked to */
    size_t m_SignalNumber;
};

#endif
