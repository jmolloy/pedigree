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
#include <Scrollable.h>
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
class LocksCommand : public DebuggerCommand,
                     public Scrollable
{
    friend class Spinlock;
public:
    /**
     * Default constructor - zeroes stuff.
     */
    LocksCommand();

    /**
     * Default destructor - does nothing.
     */
    virtual ~LocksCommand();

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

    /**
     * LocksCommand defaults to logging errors and returning failure.
     * Call this to change it to logging fatal errors itself.
     */
    void setFatal();

    bool lockAttempted(Spinlock *pLock, size_t nCpu=~0U, bool intState=false);
    bool lockAcquired(Spinlock *pLock, size_t nCpu=~0U, bool intState=false);
    bool lockReleased(Spinlock *pLock, size_t nCpu=~0U);

    /**
     * Notifies the command that we'd like to lock the given lock, allowing
     * the LocksCommand instance to detect common concurrency issues like
     * dependency inversion. This should be called after an acquire() fails,
     * as it may have undesirable overhead for the "perfect" case.
     */
    bool checkState(Spinlock *pLock, size_t nCpu=~0U);

    // Scrollable interface.
    virtual const char *getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour);
    virtual const char *getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour) ;
    virtual size_t getLineCount();

protected:
    void clearFatal();

private:
    enum State
    {
        /// The lock is about to be attempted.
        Attempted,
        /// The lock is acquired.
        Acquired,
        /// The lock failed to be acquired, and has been checked once.
        Checked,
        /// This entry is no longer active.
        Inactive,
    };

    const char *stateName(State s)
    {
        switch(s)
        {
            case Attempted:
                return "attempted";
            case Acquired:
                return "acquired";
            case Checked:
                return "checked";
            case Inactive:
                return "inactive";
            default:
                return "unknown";
        }
    }

    struct LockDescriptor
    {
        LockDescriptor() : pLock(0), ra(), n(0), state(Inactive)
        {
        }

        Spinlock *pLock;
        uintptr_t ra[NUM_BT_FRAMES];
        size_t n;
        State state;
    };

    LockDescriptor m_pDescriptors[LOCKS_COMMAND_NUM_CPU][MAX_DESCRIPTORS];

    Atomic<bool> m_bAcquiring[LOCKS_COMMAND_NUM_CPU];
    Atomic<size_t> m_NextPosition[LOCKS_COMMAND_NUM_CPU];
    Atomic<size_t> m_LockIndex;

    bool m_bFatal;

    size_t m_SelectedLine;
};

extern LocksCommand g_LocksCommand;

/** @} */
#endif
