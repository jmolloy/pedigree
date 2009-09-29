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

#ifndef SLAMCOMMAND_H
#define SLAMCOMMAND_H

#include <DebuggerCommand.h>
#include <Scrollable.h>
#include <utilities/Vector.h>
#include <utilities/Tree.h>

/** @addtogroup kerneldebuggercommands
 * @{ */

#define NUM_SLAM_BT_FRAMES 10

/**
 * Traces page allocations.
 */
class SlamCommand : public DebuggerCommand,
                          public Scrollable
{
public:
    /**
     * Default constructor - zeroes stuff.
     */
    SlamCommand();

    /**
     * Default destructor - does nothing.
     */
    ~SlamCommand();

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
        return NormalStaticString("slam-allocations");
    }

    void addAllocation(uintptr_t *backtrace, size_t requested);

    void removeAllocation(uintptr_t *backtrace, size_t requested);

    void clean()
    {
        m_Tree.clear();
    };

    //
    // Scrollable interface
    //
    virtual const char *getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour);
    virtual const char *getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour);
    virtual size_t getLineCount();

private:
    struct SlamAllocation
    {
        uintptr_t bt[NUM_SLAM_BT_FRAMES];
        size_t n;
        size_t size;
    };
    Tree<size_t, SlamAllocation*> m_Tree;
    Tree<size_t, SlamAllocation*>::Iterator m_It;
    size_t m_nLines;
    size_t m_nIdx;
    bool m_Lock;
};

extern SlamCommand g_SlamCommand;

/** @} */
#endif
