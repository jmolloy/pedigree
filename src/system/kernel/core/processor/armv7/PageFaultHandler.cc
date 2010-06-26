/*
 * Copyright (c) 2008 James Molloy, JÃ¶rg PfÃ¤hler, Matthew Iselin
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

#include <Log.h>
#include <Debugger.h>
#include <processor/PageFaultHandler.h>
#include <process/Scheduler.h>
#include <panic.h>
#include <processor/PhysicalMemoryManager.h>

/// \todo Implement

PageFaultHandler PageFaultHandler::m_Instance;

bool PageFaultHandler::initialise()
{
    return true;
}

void PageFaultHandler::interrupt(size_t interruptNumber, InterruptState &state)
{
}

PageFaultHandler::PageFaultHandler() :
    m_Handlers()
{
}
