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

#include "TcpManager.h"
#include "TcpMisc.h"
#include "TcpStateBlock.h"
#include <LockGuard.h>
#include <Log.h>

int stateBlockFree(void* p)
{
  StateBlock* stateBlock = reinterpret_cast<StateBlock*>(p);
  TcpManager::instance().removeConn(stateBlock->connId);
  return 0;
}

size_t TcpBuffer::write(uintptr_t buffer, size_t nBytes)
{
    LockGuard<Mutex> guard(m_Lock);

    // Validation
    if(!m_Buffer || !m_BufferSize)
        return 0;

    // Is the buffer full?
    if(m_DataSize >= m_BufferSize)
        return 0;

    // If adding this data will cause an overflow, limit the write
    if((nBytes + m_DataSize) > m_BufferSize)
        nBytes = m_BufferSize - m_DataSize;

    // If there's still too many bytes, restrict further
    if(nBytes > m_BufferSize)
        nBytes = m_BufferSize;

    // If however there's no more room, we can't write
    if(!nBytes)
        return 0;

    // Can we just write the whole thing?
    if((m_Writer + nBytes) < m_BufferSize)
    {
        // Yes, so just copy directly into the buffer
        MemoryCopy(reinterpret_cast<void*>(m_Buffer+m_Writer),
               reinterpret_cast<void*>(buffer),
               nBytes);
        m_Writer += nBytes;
        m_DataSize += nBytes;
        return nBytes;
    }
    else
    {
        // This write will overlap the buffer
        size_t numNormalBytes = m_BufferSize - m_Writer;
        size_t numOverlapBytes = (m_Writer + nBytes) % m_BufferSize;

        // Does the write overlap the reader position?
        if(numOverlapBytes >= m_Reader && m_Reader != 0)
            numOverlapBytes = m_Reader - 1;
        // Has the reader position progressed, at all?
        else if(m_Reader == 0 && m_DataSize == 0)
            numOverlapBytes = 0;

        // Copy the normal bytes
        if(numNormalBytes)
            MemoryCopy(reinterpret_cast<void*>(m_Buffer+m_Writer),
                   reinterpret_cast<void*>(buffer),
                   numNormalBytes);
        if(numOverlapBytes)
            MemoryCopy(reinterpret_cast<void*>(m_Buffer),
                   reinterpret_cast<void*>(buffer),
                   numOverlapBytes);

        // Update the writer position, if needed
        if(numOverlapBytes)
            m_Writer = numOverlapBytes;

        // Return the number of bytes written
        m_DataSize += numNormalBytes + numOverlapBytes;
        return numNormalBytes + numOverlapBytes;
    }
}

size_t TcpBuffer::read(uintptr_t buffer, size_t nBytes, bool bDoNotMove)
{
    LockGuard<Mutex> guard(m_Lock);

    // Verify that we will actually be able to read this data
    if(!m_Buffer || !m_BufferSize)
        return 0;

    // Do not read past the end of the allocated buffer
    if(nBytes > m_BufferSize)
        nBytes = m_BufferSize;

    // And do not read more than the data that is already in the buffer
    if(nBytes > m_DataSize)
        nBytes = m_DataSize;

    // If either of these checks cause nBytes to be zero, just return
    if(!nBytes)
        return 0;

    // Can we just read the whole thing?
    if((m_Reader + nBytes) < m_BufferSize)
    {
        // Limit the number of bytes to the writer position
        if(m_Writer == 0 && m_Reader == 0)
            return 0; // No data to read
        else if(m_Writer == m_Reader && m_DataSize == 0) // If there's data, the writer has wrapped around
            return 0; // Reader == Writer, no data
        else if(nBytes > m_DataSize)
            nBytes = m_DataSize;
        if(!nBytes)
            return 0; // No data?

        // Yes, so just copy directly into the buffer
        MemoryCopy(reinterpret_cast<void*>(buffer),
               reinterpret_cast<void*>(m_Buffer+m_Reader),
               nBytes);
        if(!bDoNotMove)
        {
            m_Reader += nBytes;
            m_DataSize -= nBytes;
        }
        return nBytes;
    }
    else
    {
        // This read will wrap around to the beginning
        size_t numNormalBytes = m_BufferSize - m_Reader;
        size_t numOverlapBytes = (m_Reader + nBytes) % m_BufferSize;

        // Does the read overlap the write position?
        if(numOverlapBytes >= m_Writer && m_Writer != 0)
            numOverlapBytes = m_Writer - 1;
        // If the writer's sitting at position 0, don't overlap
        else if(m_Writer == 0)
            numOverlapBytes = 0;

        // Copy the normal bytes
        if(numNormalBytes)
            MemoryCopy(reinterpret_cast<void*>(buffer),
                   reinterpret_cast<void*>(m_Buffer+m_Reader),
                   numNormalBytes);
        if(numOverlapBytes)
            MemoryCopy(reinterpret_cast<void*>(buffer + numNormalBytes),
                   reinterpret_cast<void*>(m_Buffer),
                   numOverlapBytes);

        // Update the writer position, if needed
        if(numOverlapBytes)
            m_Reader = numOverlapBytes;

        // Return the number of bytes written
        if((numNormalBytes + numOverlapBytes) > m_DataSize)
            m_DataSize = 0;
        else
            m_DataSize -= numNormalBytes + numOverlapBytes;
        return numNormalBytes + numOverlapBytes;
    }
}

void TcpBuffer::setSize(size_t newBufferSize)
{
    LockGuard<Mutex> guard(m_Lock);

    if(m_Buffer)
        delete [] reinterpret_cast<uint8_t*>(m_Buffer);

    if(newBufferSize)
    {
        m_BufferSize = newBufferSize;
        m_Buffer = reinterpret_cast<uintptr_t>(new uint8_t[newBufferSize + 1]);
    }
}
