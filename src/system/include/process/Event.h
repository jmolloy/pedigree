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
#ifndef EVENT_H
#define EVENT_H

#include <processor/types.h>

/** The maximum size of one event, serialized. */
#define EVENT_LIMIT       4096
/** The maximum thread ID one can dispatch an Event to. */
#define EVENT_TID_MAX     255
/** The maximum number of times an event can fire while in an event handler (nesting levels).
    After this, the process will be terminated. */
#define MAX_NESTED_EVENTS 16

#if defined(X86) || defined(X64) || defined(PPC32)
  #define EVENT_BASE                0x80000000
#elif defined(ARMV7)
  #define EVENT_BASE                0xF0000000
#else
  #define EVENT_BASE                0
#endif

#define EVENT_HANDLER_TRAMPOLINE  (EVENT_BASE+0)
#define EVENT_HANDLER_TRAMPOLINE2 (EVENT_BASE+0x100) /// \todo Temporary fix so I can test my pthreads stuff...
#define EVENT_HANDLER_BUFFER      (EVENT_BASE+0x1000)
#define EVENT_HANDLER_END         (EVENT_HANDLER_BUFFER + (EVENT_TID_MAX*MAX_NESTED_EVENTS)*EVENT_LIMIT)

/** The abstract base class for an asynchronous event. An event can hold any amount of information
    up to a hard maximum size of EVENT_LIMIT (usually 4096 bytes). An event is serialized using 
    the virtual serialize() function and sent to a recipient thread in either user or kernel mode,
    where it is unserialized. */
class Event
{
public:
    /** Constructs an Event object.
        \param handlerAddress The address of the handling function.
        \param isDeletable Can the object be deleted after map()? This is used for creating objects
                           without worrying about destroying them.
        \param specificNestingLevel Is the event pinned to a specific nesting level? If this value is not ~0UL,
                                    then the event will only be fired if the current nesting level is 
                                    \p specificNestingLevel .
        \note As can be surmised, handlerAddress is NOT reentrant. If you use this Event in multiple
              threads concurrently, you CANNOT change the handler address. */
    Event(uintptr_t handlerAddress, bool isDeletable, size_t specificNestingLevel=~0UL);

    virtual ~Event();

    /** Returns true if the event is on the heap and can be deleted when handled. This is for creating
        fire-and-forget messages and not worrying about memory leaks. */
    virtual bool isDeletable();

    /** Given a buffer EVENT_LIMIT bytes long, take the variables
        present in this object and convert them to a binary form.

        It can be assumed that this function will only be called when
        the Event is about to be dispatched, and so the inhibited mask
        of the current Thread is available to be changed to what the
        event handler requires. This change will be undone when the
        handler completes.

        \param pBuffer The buffer to serialize to.
        \return The number of bytes serialized. */
    virtual size_t serialize(uint8_t *pBuffer) = 0;

    /** Given a serialized Event in binary form, attempt to unserialize into the given object.
        If this is impossible, return false.

        \note It is impossible to make static functions virtual, but it is <b>required</b> that 
              all subclasses of Event implement this function statically. */
    static bool unserialize(uint8_t *pBuffer, Event &event);

    /** Given a serialized event, returns the type of that event (a constant from eventNumbers.h). */
    static size_t getEventType(uint8_t *pBuffer);

    /** Returns the handler address. */
    uintptr_t getHandlerAddress() {return m_HandlerAddress;}

    /** Returns the specific nesting level, or ~0UL if there is none defined. */
    size_t getSpecificNestingLevel() {return m_NestingLevel;}

    /** Returns the event number / ID. */
    virtual size_t getNumber() = 0;

protected:
    /** Handler address. */
    uintptr_t m_HandlerAddress;

    /** Can the object be deleted after map? */
    bool m_bIsDeletable;

    /** Specific nesting level, or ~0UL. */
    size_t m_NestingLevel;
};

#endif
