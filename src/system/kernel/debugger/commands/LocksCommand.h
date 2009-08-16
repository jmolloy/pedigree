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

#ifndef LOCKSCOMMAND_H
#define LOCKSCOMMAND_H

#include <DebuggerCommand.h>
#include <Spinlock.h>

/** @addtogroup kerneldebuggercommands
 * @{ */

#define NUM_BT_FRAMES 6
#define MAX_DESCRIPTORS 50

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

    void lockAcquired(Spinlock *pLock);
    void lockReleased(Spinlock *pLock);
  
private:
    struct LockDescriptor
    {
        Spinlock *pLock;
        uintptr_t ra[NUM_BT_FRAMES];
        size_t n;
        bool used;
    };
    LockDescriptor m_pDescriptors[MAX_DESCRIPTORS];

    bool m_bAcquiring;
};

extern LocksCommand g_LocksCommand;

/** @} */
#endif
