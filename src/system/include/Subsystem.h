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
#ifndef SUBSYSTEM_H
#define SUBSYSTEM_H

// Forward definition of used classes
class Thread;

#include <processor/state.h>

/** The abstract base class for a generic application subsystem. This provides
  * a well-defined interface to the kernel that allows global behaviour to have
  * correct results on different applications. This also allows the kernel to
  * keep subsystem-specific code to a minimum.
  *
  * Basically, when inheriting from this class, you are creating a layer between
  * your subsystem and the kernel.
  */
class Subsystem
{
    public:

        /** Defines the different types of subsystems */
        enum SubsystemType
        {
            Posix = 0,
            Native = 1,
            None = 255
        };

        /** Type of exception.
          * This is passed to the subsystem when a Thread throws an exception,
          * which allows subsystem-specific behaviour to be performed.
          */
        enum ExceptionType
        {
            InvalidOpcode = 0,
            PageFault = 1,
            Other = 255
        };

        /** Default constructor */
        Subsystem() :
            m_Type(None)
        {}

        /** Copy constructor */
        Subsystem(const Subsystem &s) :
            m_Type(s.m_Type)
        {}

        /** Parameterised constructor */
        Subsystem(SubsystemType type) :
            m_Type(type)
        {}

        /** Default destructor */
        virtual ~Subsystem()
        {}

        /** A thread needs to be killed! */
        virtual bool kill(Thread *pThread);

        /** A thread has thrown an exception! */
        virtual void threadException(Thread *pThread, ExceptionType eType, InterruptState &state);

    protected:

        SubsystemType m_Type;
};

#endif

