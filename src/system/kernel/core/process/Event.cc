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
#include <process/Event.h>

#include <Log.h>

Event::Event(uintptr_t handlerAddress, bool isDeletable, size_t specificNestingLevel) :
    m_HandlerAddress(handlerAddress), m_bIsDeletable(isDeletable), m_NestingLevel(specificNestingLevel)
{
}

Event::~Event()
{
}

bool Event::isDeletable()
{
    return m_bIsDeletable;
}

bool Event::unserialize(uint8_t *pBuffer, Event &event)
{
    ERROR("Event::unserialize is abstract, should never be called.");
    return false;
}

size_t Event::getEventType(uint8_t *pBuffer)
{
    size_t *pBufferSize_t = reinterpret_cast<size_t*> (pBuffer);
    return pBufferSize_t[0];
}
