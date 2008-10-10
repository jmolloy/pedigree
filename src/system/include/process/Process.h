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
#include <utilities/Tree.h>

class VirtualAddressSpace;

/**
 * An abstraction of a Process - a container for one or more threads all running in
 * the same address space.
 */
class Process
{
public:
  /** Default constructor. */
  Process();

  /** Constructor for creating a new Process. Creates a new Process as
   * a UNIX fork() would, from the given parent process. This constructor
   * does not create any threads.
   * \param pParent The parent process. */
  Process(Process *pParent);
  
  /** Destructor. */
  ~Process();
  
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
  
  /** Returns the File descriptor map - maps numbers to pointers (of undefined type -
      the subsystem decides what type). */
  Tree<size_t,void*> &getFdMap()
  {
    return m_FdMap;
  }

  /** Returns the next available file descriptor. */
  size_t nextFd()
  {
    LockGuard<Spinlock> guard(m_FdLock); // Must be atomic.
    return m_NextFd++;
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
  String &getCwd()
  {
    return m_Cwd;
  }
  /** Sets the current working directory. */
  void setCwd(String str)
  {
    m_Cwd = str;
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
   * The file descriptor map. Maps number to pointers, the type of which is decided
   * by the subsystem.
   */
  Tree<size_t,void*> m_FdMap;
  /**
   * The next available file descriptor.
   */
  size_t m_NextFd;
  /**
   * Lock to guard the next file descriptor while it's being changed.
   */
  Spinlock m_FdLock;
  /**
   * Process exit status.
   */
  int m_ExitStatus;
  /**
   * Current working directory.
   */
  String m_Cwd;
};

#endif
