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

#ifndef _PROCESS_TIME_TRACKER_H
#define _PROCESS_TIME_TRACKER_H

/**
 * Tracks time spent and assigns it to the given process, in an RAII way.
 *
 * Mostly useful for things like syscalls where there's a definitive entry and
 * exit at which time should be tracked differently.
 */
class TimeTracker
{
public:
    TimeTracker(Process *pProcess, bool fromUserspace) :
        m_pProcess(pProcess), m_bFromUserspace(fromUserspace)
    {
        if (m_pProcess == 0)
        {
            // We can get called early, so ensure we don't make any
            // assumptions about what's present.
            Thread *pThread = Processor::information().getCurrentThread();
            if (!pThread)
                return;
            m_pProcess = pThread->getParent();
            if (!m_pProcess)
                return;
        }

        // Track time already spent wherever we were previously.
        m_pProcess->trackTime(m_bFromUserspace);

        // Record current time for the next tracking.
        m_pProcess->recordTime(!m_bFromUserspace);
    }

    virtual ~TimeTracker()
    {
        if (!m_pProcess)
            return;

        // Track time spent in the RAII section.
        m_pProcess->trackTime(!m_bFromUserspace);

        // Record current time for future tracking.
        m_pProcess->recordTime(m_bFromUserspace);
    }

private:
    Process *m_pProcess;
    bool m_bFromUserspace;
};

#endif
