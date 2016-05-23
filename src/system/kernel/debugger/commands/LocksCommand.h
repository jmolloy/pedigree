/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef LOCKSCOMMAND_H
#define LOCKSCOMMAND_H

#include <DebuggerCommand.h>
#include <Spinlock.h>

/** @addtogroup kerneldebuggercommands
 * @{ */

#define NUM_BT_FRAMES 6
#define MAX_DESCRIPTORS 50

#ifdef TESTSUITE
#define LOCKS_COMMAND_NUM_CPU 4
#else
#ifdef MULTIPROCESSOR
#define LOCKS_COMMAND_NUM_CPU 255
#else
#define LOCKS_COMMAND_NUM_CPU 1
#endif
#endif

/**
 * Traces lock allocations.
 */
class LocksCommand : public DebuggerCommand
{
public:
    /**
     * Default constructor - zeroes stuff.
     */
    LocksCommand();

    /**
     * Default destructor - does nothing.
     */
    ~LocksCommand();

    /**
     * Return an autocomplete string, given an input string.
     */
    void autocomplete(const HugeStaticString &input, HugeStaticString &output);

    /**
     * Execute the command with the given screen.
     */
    bool execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *screen);
  
    /**
     * Returns the string representation of this command.
     */
    const NormalStaticString getString()
    {
        return NormalStaticString("locks");
    }

    void setReady();

    bool lockAttempted(Spinlock *pLock, size_t nCpu=~0);
    bool lockAcquired(Spinlock *pLock, size_t nCpu=~0);
    bool lockReleased(Spinlock *pLock, size_t nCpu=~0);

    /**
     * Notifies the command that we'd like to lock the given lock, allowing
     * the LocksCommand instance to detect common concurrency issues like
     * dependency inversion. This should be called after an acquire() fails,
     * as it may have undesirable overhead for the "perfect" case.
     */
    bool checkState(Spinlock *pLock, size_t nCpu=~0);
  
private:
    struct LockDescriptor
    {
        Spinlock *pLock;
        uintptr_t ra[NUM_BT_FRAMES];
        size_t n;
        bool used;
        bool attempt;
        size_t index;
    };

    LockDescriptor m_pDescriptors[LOCKS_COMMAND_NUM_CPU][MAX_DESCRIPTORS];

    Atomic<bool> m_bAcquiring[LOCKS_COMMAND_NUM_CPU];
    Atomic<size_t> m_LockIndex;
};

extern LocksCommand g_LocksCommand;

/** @} */
#endif
