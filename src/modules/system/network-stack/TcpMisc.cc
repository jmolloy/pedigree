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

#ifndef TESTSUITE
#include "TcpManager.h"
#include "TcpStateBlock.h"
#endif

#include "TcpMisc.h"
#include <Log.h>

#ifdef THREADS
#include <LockGuard.h>
#endif

#ifndef TESTSUITE
int stateBlockFree(void* p)
{
  StateBlock* stateBlock = reinterpret_cast<StateBlock*>(p);
  TcpManager::instance().removeConn(stateBlock->connId);
  return 0;
}
#endif

void TcpBuffer::newSegment(uintptr_t buffer, size_t size)
{
    // Just add the segment.
    Segment *pSegment = new Segment;
    MemoryCopy(pSegment->buffer, reinterpret_cast<void *>(buffer), size);
    pSegment->size = size;

    m_Segments.pushBack(pSegment);
    m_DataSize += size;
}

size_t TcpBuffer::write(uintptr_t buffer, size_t nBytes)
{
#ifdef THREADS
    LockGuard<Mutex> guard(m_Lock);
#endif

    // Don't exceed the buffer size, taking into account the current data size.
    if (nBytes > getRemainingSize())
    {
        nBytes = getRemainingSize();
    }

    // Don't exceed more than one segment's worth of data in one write();
    // this ensures we can always just push a new segment if we need to.
    if (nBytes > m_SegmentBufferSize)
    {
        nBytes = m_SegmentBufferSize;
    }

    // Do we need a new segment? Alternatively, if we're pushing a full-size
    // segment, don't bother splitting it across existing segments - just add
    // a new one.
    if ((m_Segments.begin() == m_Segments.end()) ||
        (nBytes == m_SegmentBufferSize))
    {
        newSegment(buffer, nBytes);
        return nBytes;
    }

    Segment *pSegment = m_Segments.popBack();
    if (pSegment->size == m_SegmentBufferSize)
    {
        // Full segment, need to make a new one and push it.
        m_Segments.pushBack(pSegment);
        newSegment(buffer, nBytes);
        return nBytes;
    }

    // We have room in this buffer.
    uint8_t *start = adjust_pointer(pSegment->buffer, pSegment->size);
    size_t copyableSize = m_SegmentBufferSize - pSegment->size;
    if (copyableSize > nBytes)
        copyableSize = nBytes;
    MemoryCopy(start, reinterpret_cast<void *>(buffer), copyableSize);
    pSegment->size += copyableSize;
    m_DataSize += copyableSize;

    // Adjusted, push again.
    m_Segments.pushBack(pSegment);

    // Do we need a new segment after this one for any leftover?
    if (copyableSize < nBytes)
    {
        newSegment(buffer + copyableSize, nBytes - copyableSize);
    }

    return nBytes;
}

size_t TcpBuffer::readSegment(Segment *pSegment, uintptr_t target, size_t size, bool bUpdate)
{
    // Count of bytes present that have not yet been read.
    size_t availableBytes = pSegment->size - pSegment->reader;

    // Count of bytes to actually read from this segment.
    size_t bytesToRead = size;
    if (bytesToRead > availableBytes)
        bytesToRead = availableBytes;

    MemoryCopy(reinterpret_cast<void *>(target),
               adjust_pointer(pSegment->buffer, pSegment->reader), bytesToRead);

    if (bUpdate)
    {
        pSegment->reader += bytesToRead;
        m_DataSize -= bytesToRead;
    }

    return bytesToRead;
}

size_t TcpBuffer::read(uintptr_t buffer, size_t nBytes, bool bDoNotMove)
{
    // Different logic for move vs non-move.
    if (bDoNotMove)
    {
        size_t totalRead = 0;
        for (auto it : m_Segments)
        {
            totalRead += readSegment(it, buffer + totalRead,
                                     nBytes - totalRead, false);
        }

        return totalRead;
    }
    else
    {
        size_t totalRead = 0;
        while (m_Segments.count() && totalRead < nBytes)
        {
            Segment *pSegment = m_Segments.popFront();
            totalRead += readSegment(pSegment, buffer + totalRead,
                                     nBytes - totalRead, true);

            // Did we not completely read this segment?
            if (pSegment->reader < pSegment->size)
            {
                // Yes - still bytes to be read.
                m_Segments.pushFront(pSegment);
            }
            else
            {
                delete pSegment;
            }
        }

        return totalRead;
    }
}

void TcpBuffer::setSize(size_t newBufferSize)
{
#ifdef THREADS
    LockGuard<Mutex> guard(m_Lock);
#endif

    m_BufferSize = newBufferSize;

    // Clear out all existing segments to resize.
    for (auto it : m_Segments)
    {
        delete it;
    }
    m_Segments.clear();
}
