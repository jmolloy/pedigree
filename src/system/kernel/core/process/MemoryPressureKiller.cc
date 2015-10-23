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

#include <process/MemoryPressureKiller.h>
#include <process/Process.h>
#include <process/Scheduler.h>
#include <Subsystem.h>
#include <Log.h>

static size_t mb(size_t pages)
{
    return (pages * 0x1000) / 0x100000;
}

bool MemoryPressureProcessKiller::compact()
{
    Process *pCandidateProcess = 0;
    for(size_t i = 0; i < Scheduler::instance().getNumProcesses(); ++i)
    {
        Process *pProcess = Scheduler::instance().getProcess(i);

        // Requires a subsystem to kill.
        if (!pProcess->getSubsystem())
            continue;

        if (!pCandidateProcess)
            pCandidateProcess = pProcess;
        else
        {
            if (pProcess->getPhysicalPageCount() >
                pCandidateProcess->getPhysicalPageCount())
            {
                pCandidateProcess = pProcess;
            }
        }
    }

    if (!pCandidateProcess)
        return false;

    NOTICE_NOLOCK("MemoryPressureProcessKiller will kill pid=" << Dec << pCandidateProcess->getId() << Hex);
    NOTICE_NOLOCK("virt=" << Dec << mb(pCandidateProcess->getVirtualPageCount()) << "m phys=" << mb(pCandidateProcess->getPhysicalPageCount()) << "m shared=" << mb(pCandidateProcess->getSharedPageCount()) << "m" << Hex);

    Subsystem *pSubsystem = pCandidateProcess->getSubsystem();
    pSubsystem->threadException(pCandidateProcess->getThread(0), Subsystem::Quit);

    // Give the process time to quit.
    Scheduler::instance().yield();

    return true;
}
