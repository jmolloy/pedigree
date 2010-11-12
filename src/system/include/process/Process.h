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

#ifndef PROCESS_H
#define PROCESS_H

#include <process/Thread.h>
#include <processor/state.h>
#include <utilities/Vector.h>
#include <utilities/StaticString.h>
#include <utilities/String.h>
#include <Atomic.h>
#include <Spinlock.h>
#include <LockGuard.h>
#include <process/Semaphore.h>
#include <process/Mutex.h>
#include <utilities/Tree.h>
#include <utilities/MemoryAllocator.h>

#include <Subsystem.h>

class VirtualAddressSpace;
class File;
class FileDescriptor;
class User;
class Group;
class DynamicLinker;

/**
 * An abstraction of a Process - a container for one or more threads all running in
 * the same address space.
 */
class Process
{
public:
    /** Subsystems may inherit Process to provide custom functionality. However, they need
     *  to know whether a Process pointer is subsystem-specific. This enumeration is designed
     *  to allow functions using Process objects in subsystems with inherited Process objects
     *  to be able to figure out what type the Process is without depending on any external
     *  accounting.
     */
    enum ProcessType
    {
        Stock,
        Posix
    };

    /** Default constructor. */
    Process();

    /** Constructor for creating a new Process. Creates a new Process as
     * a UNIX fork() would, from the given parent process. This constructor
     * does not create any threads.
     * \param pParent The parent process. */
    Process(Process *pParent);

    /** Destructor. */
    virtual ~Process();

    /** Adds a thread to this process.
     *  \return The thread ID to be assigned to the new Thread. */
    size_t addThread(Thread *pThread);
    /** Removes a thread from this process. */
    void removeThread(Thread *pThread);

    /** Returns the number of threads in this process. */
    size_t getNumThreads();
    /** Returns the n'th thread in this process. */
    Thread *getThread(size_t n);

    /** Creates a new process, with a single thread and a stack. */
    static uintptr_t create(uint8_t *elf, size_t elfSize, const char *name);

    /** Returns the process ID. */
    size_t getId()
    {
        return m_Id;
    }

    /** Returns the description string of this process. */
    LargeStaticString &description()
    {
        return str;
    }

    /** Returns our address space */
    VirtualAddressSpace *getAddressSpace()
    {
        return m_pAddressSpace;
    }

    /** Sets the exit status of the process. */
    void setExitStatus(int code)
    {
        m_ExitStatus = code;
    }
    /** Gets the exit status of the process. */
    int getExitStatus()
    {
        return m_ExitStatus;
    }

    /** Kills the process. */
    void kill();

    /** Returns the parent process. */
    Process *getParent()
    {
        return m_pParent;
    }

    /** Returns the current working directory. */
    File *getCwd()
    {
        return m_Cwd;
    }
    /** Sets the current working directory. */
    void setCwd(File *f)
    {
        m_Cwd = f;
    }

    /** Returns the current controlling terminal. */
    File *getCtty()
    {
        return m_Ctty;
    }
    /** Sets the controlling terminal. */
    void setCtty(File *f)
    {
        m_Ctty = f;
    }


    /** Returns the memory space allocator for shared libraries. */
    MemoryAllocator &getSpaceAllocator()
    {
        return m_SpaceAllocator;
    }

    /** Gets the current user. */
    User *getUser()
    {
        return m_pUser;
    }
    /** Sets the current user. */
    void setUser(User *pUser)
    {
        m_pUser = pUser;
    }
    
    /** Gets the effective user. */
    User *getEffectiveUser()
    {
        return m_pEffectiveUser;
    }
    /** Sets the effective user. */
    void setEffectiveUser(User *pUser)
    {
        m_pEffectiveUser = pUser;
    }

    /** Gets the current group. */
    Group *getGroup()
    {
        return m_pGroup;
    }
    /** Sets the current group. */
    void setGroup(Group *pGroup)
    {
        m_pGroup = pGroup;
    }
    
    /** Gets the current effective group. */
    Group *getEffectiveGroup()
    {
        return m_pEffectiveGroup;
    }
    void setEffectiveGroup(Group *pGroup)
    {
        m_pEffectiveGroup = pGroup;
    }

    void setLinker(DynamicLinker *pDl)
    {
        m_pDynamicLinker = pDl;
    }
    DynamicLinker *getLinker()
    {
        return m_pDynamicLinker;
    }

    void setSubsystem(Subsystem *pSubsystem)
    {
        m_pSubsystem = pSubsystem;
    }
    Subsystem *getSubsystem()
    {
        return m_pSubsystem;
    }

    /** Gets the type of the Process (subsystems may override) */
    virtual ProcessType getType()
    {
        return Stock;
    }

private:
    Process(const Process &);
    Process &operator = (const Process &);

    /**
     * Our list of threads.
     */
    Vector<Thread*> m_Threads;
    /**
     * The next available thread ID.
     */
    Atomic<size_t> m_NextTid;
    /**
     * Our Process ID.
     */
    size_t m_Id;
    /**
     * Our description string.
     */
    LargeStaticString str;
    /**
     * Our parent process.
     */
    Process *m_pParent;
    /**
     * Our virtual address space.
     */
    VirtualAddressSpace *m_pAddressSpace;
    /**
     * Process exit status.
     */
    int m_ExitStatus;
    /**
     * Current working directory.
     */
    File *m_Cwd;
    /**
     * Current controlling terminal.
     */
    File *m_Ctty;
    /**
     * Memory allocator for shared libraries - free parts of the address space.
     */
    MemoryAllocator m_SpaceAllocator;
    /** Current user. */
    User *m_pUser;
    /** Current group. */
    Group *m_pGroup;
    /** Effective user. */
    User *m_pEffectiveUser;
    /** Effective group. */
    Group *m_pEffectiveGroup;

    /** The Process' dynamic linker. */
    DynamicLinker *m_pDynamicLinker;

    /** The subsystem for this process */
    Subsystem *m_pSubsystem;

public:
    Semaphore m_DeadThreads;
};

#endif
