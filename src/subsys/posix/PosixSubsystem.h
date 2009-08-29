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
#include <process/Semaphore.h>

#include <utilities/Tree.h>
#include <utilities/RadixTree.h>
#include <utilities/ExtensibleBitmap.h>
#include <LockGuard.h>

class File;
class LockedFile;
class MemoryMappedFile;

/** A map linking full paths to (advisory) locked files */
/// \todo Locking!
extern RadixTree<LockedFile*> g_PosixGlobalLockedFiles;

/** Abstraction of a file descriptor, which defines an open file
  * and related flags.
  */
class FileDescriptor
{
    public:

        /// Default constructor
        FileDescriptor();

        /// Parameterised constructor
        FileDescriptor(File *newFile, uint64_t newOffset = 0, size_t newFd = 0xFFFFFFFF, int fdFlags = 0, int flFlags = 0, LockedFile *lf = 0);

        /// Copy constructor
        FileDescriptor(FileDescriptor &desc);

        /// Pointer copy constructor
        FileDescriptor(FileDescriptor *desc);

        /// Assignment operator implementation
        FileDescriptor &operator = (FileDescriptor &desc);

        /// Destructor - decreases file reference count
        virtual ~FileDescriptor();

        /// Our open file pointer
        File* file;

        /// Offset within the file for I/O
        uint64_t offset;

        /// Descriptor number
        size_t fd;

        /// File descriptor flags (fcntl)
        int fdflags;

        /// File status flags (fcntl)
        int flflags;

        /// Locked file, non-zero if there is an advisory lock on the file
        LockedFile *lockedFile;
};

/** Defines the compatibility layer for the POSIX Subsystem */
class PosixSubsystem : public Subsystem
{
    public:

        /** Default constructor */
        PosixSubsystem() :
            Subsystem(Posix), m_SignalHandlers(), m_SignalHandlersLock(false),
            m_FdMap(), m_NextFd(0), m_FdLock(false), m_FdBitmap(), m_LastFd(0), m_FreeCount(1),
            m_MemoryMappedFiles(), m_AltSigStack(), m_Semaphores(), m_Threads()
        {}

        /** Copy constructor */
        PosixSubsystem(PosixSubsystem &s);

        /** Parameterised constructor */
        PosixSubsystem(SubsystemType type) :
            Subsystem(type), m_SignalHandlers(), m_SignalHandlersLock(false),
            m_FdMap(), m_NextFd(0), m_FdLock(false), m_FdBitmap(), m_LastFd(0), m_FreeCount(1),
            m_MemoryMappedFiles(), m_AltSigStack(), m_Semaphores(), m_Threads()
        {}

        /** Default destructor */
        virtual ~PosixSubsystem();

        /** A thread needs to be killed! */
        virtual bool kill(Thread *pThread);

        /** A thread has thrown an exception! */
        virtual void threadException(Thread *pThread, ExceptionType eType, InterruptState &state);

        /** Alternate signal stack */
        /// \todo Figure out how to make this work for more than just the current process (ie, work
        ///       with CheckEventState... Which requires exposing parts of the POSIX subsystem to
        ///       the scheduler - not good!).
        struct AlternateSignalStack
        {
            /// Default constructor
            AlternateSignalStack() : base(0), size(0), inUse(false), enabled(false)
            {};

            /// Default destructor
            virtual ~AlternateSignalStack()
            {};

            /// The location of this stack
            uintptr_t base;

            /// Size of the stack
            size_t size;

            /// Are we to use this alternate stack rather than a normal stack?
            bool inUse;

            /// Enabled?
            bool enabled;
        };

        /** Grabs the alternate signal stack */
        AlternateSignalStack &getAlternateSignalStack()
        {
            return m_AltSigStack;
        }

        /** Sets the alternate signal stack, if possible */
        void setAlternateSignalStack(AlternateSignalStack &s)
        {
            m_AltSigStack = s;
        }

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

        void exit(int code);

        /** Copies file descriptors from another subsystem */
        bool copyDescriptors(PosixSubsystem *pSubsystem);

        /** Returns the first available file descriptor. */
        size_t getFd();

        /** Sets the given file descriptor as "in use". */
        void allocateFd(size_t fdNum);

        /** Sets the given file descriptor as "available" and deletes the FileDescriptor
          * linked to it.
          */
        void freeFd(size_t fdNum);

        /** Frees a range of descriptors (or only those marked FD_CLOEXEC) */
        void freeMultipleFds(bool bOnlyCloExec = false, size_t iFirst = 0, size_t iLast = -1);

        /** Gets a pointer to a FileDescriptor object from an fd number */
        FileDescriptor *getFileDescriptor(size_t fd)
        {
            LockGuard<Mutex> guard(m_FdLock);
            return m_FdMap.lookup(fd);
        }

        /** Inserts a file descriptor */
        void addFileDescriptor(size_t fd, FileDescriptor *pFd)
        {
            freeFd(fd);
            allocateFd(fd);

            LockGuard<Mutex> guard(m_FdLock);
            m_FdMap.insert(fd, pFd);
        }

        /** Links a MemoryMappedFile */
        void memoryMapFile(void* p, MemoryMappedFile *m)
        {
            if(m_MemoryMappedFiles.lookup(p))
                return;
            m_MemoryMappedFiles.insert(p, m);
        }

        /** Removes a MemoryMappedFile link */
        MemoryMappedFile *unmapFile(void* p)
        {
            MemoryMappedFile *ret = m_MemoryMappedFiles.lookup(p);
            m_MemoryMappedFiles.remove(p);
            return ret;
        }

        /** Gets a semaphore given a descriptor */
        Semaphore *getSemaphore(size_t n)
        {
            return m_Semaphores.lookup(n);
        }

        /** Inserts a semaphore given a descriptor and a Semaphore */
        void insertSemaphore(size_t n, Semaphore *sem)
        {
            Semaphore *t = m_Semaphores.lookup(n);
            if(t)
            {
                m_Semaphores.remove(n);
                delete t;
            }

            m_Semaphores.insert(n, sem);
        }

        /** Removes a semaphore given a descriptor */
        void removeSemaphore(size_t n)
        {
            Semaphore *t = m_Semaphores.lookup(n);
            if(t)
            {
                m_Semaphores.remove(n);
                delete t;
            }
        }

        /** POSIX Thread information */
        class PosixThread
        {
            public:
                PosixThread() : pThread(0), isRunning(true), returnValue(0), canReclaim(false), isDetached(false) {};
                virtual ~PosixThread() {};

                Thread *pThread;
                Mutex isRunning;
                void *returnValue;

                bool canReclaim;
                bool isDetached;

            private:
                PosixThread(const PosixThread &);
                const PosixThread& operator = (const PosixThread &);
        };

        /** Gets a thread given a descriptor */
        PosixThread *getThread(size_t n)
        {
            return m_Threads.lookup(n);
        }

        /** Inserts a thread given a descriptor and a Thread */
        void insertThread(size_t n, PosixThread *thread)
        {
            PosixThread *t = m_Threads.lookup(n);
            if(t)
                m_Threads.remove(n); /// \todo It might be safe to delete the pointer... We'll see.
            return m_Threads.insert(n, thread);
        }

        /** Removes a thread given a descriptor */
        void removeThread(size_t n)
        {
            m_Threads.remove(n); /// \todo It might be safe to delete the pointer... We'll see.
        }

    private:

        /** Signal handlers */
        Tree<size_t, SignalHandler*> m_SignalHandlers;

        /** A lock for access to the signal handlers tree */
        Mutex m_SignalHandlersLock;

        /**
         * The file descriptor map. Maps number to pointers, the type of which is decided
         * by the subsystem.
         */
        Tree<size_t, FileDescriptor*> m_FdMap;
        /**
         * The next available file descriptor.
         */
        size_t m_NextFd;
        /**
         * Lock to guard the next file descriptor while it is being changed.
         */
        Mutex m_FdLock;
        /**
         * File descriptors used by this process
         */
        ExtensibleBitmap m_FdBitmap;
        /**
         * Last known unallocated descriptor
         */
        size_t m_LastFd;
        /**
         * Number of times freed
         */
        int m_FreeCount;
        /**
         * Links addresses to their MemoryMappedFiles.
         */
        Tree<void*, MemoryMappedFile*> m_MemoryMappedFiles;
        /**
         * Alternate signal stack - if defined, used instead of a system-defined stack
         */
        AlternateSignalStack m_AltSigStack;
        /**
         * Links some file descriptors to Semaphores.
         */
        Tree<size_t, Semaphore*> m_Semaphores;
        /**
         * Links some file descriptors to Threads.
         */
        Tree<size_t, PosixThread*> m_Threads;
};

#endif
