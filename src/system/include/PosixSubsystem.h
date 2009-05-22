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
#ifndef POSIX_SUBSYSTEM_H
#define POSIX_SUBSYSTEM_H

#include <Subsystem.h>
#include <processor/types.h>
#include <process/SignalEvent.h>
#include <process/Mutex.h>

#include <utilities/Tree.h>
#include <LockGuard.h>

class FileDescriptor;

/** Defines the compatibility layer for the POSIX Subsystem */
class PosixSubsystem : public Subsystem
{
    public:

        /** Default constructor */
        PosixSubsystem() :
            Subsystem(Posix), m_SignalHandlers(), m_SignalHandlersLock(false), m_FdMap(), m_NextFd(0), m_FdLock()
        {}

        /** Copy constructor */
        PosixSubsystem(PosixSubsystem &s);

        /** Parameterised constructor */
        PosixSubsystem(SubsystemType type) :
            Subsystem(type), m_SignalHandlers(), m_SignalHandlersLock(false), m_FdMap(), m_NextFd(0), m_FdLock()
        {}

        /** Default destructor */
        virtual ~PosixSubsystem();

        /** A thread needs to be killed! */
        virtual bool kill(Thread *pThread);

        /** A thread has thrown an exception! */
        virtual void threadException(Thread *pThread, ExceptionType eType, InterruptState &state);

        /** A signal handler */
        struct SignalHandler
        {
            SignalHandler() :
                sig(255), pEvent(0), sigMask(0), flags(0), type(0)
            {}

            SignalHandler(const SignalHandler &s) :
                sig(s.sig), pEvent(new SignalEvent(*(s.pEvent))), sigMask(s.sigMask), flags(s.flags), type(s.type)
            {}

            ~SignalHandler()
            {
                if(pEvent)
                    delete pEvent;
            }

            SignalHandler &operator = (const SignalHandler &s)
            {
                sig = s.sig;
                pEvent = new SignalEvent(*(s.pEvent));
                sigMask = s.sigMask;
                flags = s.flags;
                type = s.type;
                return *this;
            }

            /// Signal number
            size_t sig;

            /// Event for the signal handler
            SignalEvent *pEvent;

            /// Signal mask to set when this signal handler is called
            uint32_t sigMask;

            /// Signal handler flags
            uint32_t flags;

            /// Type - 0 = normal, 1 = SIG_DFL, 2 = SIG_IGN
            int type;
        };

        /** Sets a signal handler */
        void setSignalHandler(size_t sig, SignalHandler* handler);

        /** Gets a signal handler */
        SignalHandler* getSignalHandler(size_t sig)
        {
            LockGuard<Mutex> guard(m_SignalHandlersLock);

            return reinterpret_cast<SignalHandler*>(m_SignalHandlers.lookup(sig % 32));
        }

        /** Returns the File descriptor map - maps numbers to pointers (of undefined type -
            the subsystem decides what type). */
        Tree<size_t,FileDescriptor*> &getFdMap()
        {
            return m_FdMap;
        }

        /** Returns the next available file descriptor. */
        size_t nextFd()
        {
            LockGuard<Spinlock> guard(m_FdLock); // Must be atomic.
            return m_NextFd++;
        }
        size_t nextFd(size_t fdNum)
        {
            LockGuard<Spinlock> guard(m_FdLock); // Must be atomic.
            return (m_NextFd = fdNum);
        }

    private:

        /** Signal handlers */
        Tree<size_t, void*> m_SignalHandlers;

        /** A lock for access to the signal handlers tree */
        Mutex m_SignalHandlersLock;

        /**
         * The file descriptor map. Maps number to pointers, the type of which is decided
         * by the subsystem.
         */
        Tree<size_t,FileDescriptor*> m_FdMap;
        /**
         * The next available file descriptor.
         */
        size_t m_NextFd;
        /**
         * Lock to guard the next file descriptor while it is being changed.
         */
        Spinlock m_FdLock;
};

#endif
