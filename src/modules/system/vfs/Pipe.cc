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

#include "Pipe.h"
#include "Filesystem.h"
#include <utilities/ZombieQueue.h>

class ZombiePipe : public ZombieObject
{
    public:
        ZombiePipe(Pipe *pPipe) : m_pPipe(pPipe)
        {
        }
        virtual ~ZombiePipe()
        {
            delete m_pPipe;
        }
    private:
        Pipe *m_pPipe;
};

Pipe::Pipe() :
    File(), m_bIsAnonymous(true), m_bIsEOF(false), m_BufLen(0),
    m_BufAvailable(PIPE_BUF_MAX), m_Front(0), m_Back(0)
{
}

Pipe::Pipe(String name, Time accessedTime, Time modifiedTime, Time creationTime,
           uintptr_t inode, Filesystem *pFs, size_t size, File *pParent,
           bool bIsAnonymous) :
    File(name,accessedTime,modifiedTime,creationTime,inode,pFs,size,pParent),
    m_bIsAnonymous(bIsAnonymous), m_bIsEOF(false), m_BufLen(0),
    m_BufAvailable(PIPE_BUF_MAX), m_Front(0), m_Back(0)
{
}

Pipe::~Pipe()
{
}

int Pipe::select(bool bWriting, int timeout)
{
    if(timeout)
    {
        if(bWriting)
        {
            if(m_BufAvailable.acquire(1, timeout))
            {
                m_BufAvailable.release();
                return true;
            }
        }
        else
        {
            if(m_BufLen.acquire(1, timeout))
            {
                m_BufLen.release();
                return true;
            }
        }
    }
    else
    {
        if(bWriting)
        {
            if(m_BufAvailable.tryAcquire())
            {
                m_BufAvailable.release();
                return true;
            }
        }
        else
        {
            if(m_BufLen.tryAcquire())
            {
                m_BufLen.release();
                return true;
            }
        }
    }

    return false;
}

uint64_t Pipe::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    uint64_t n = 0;
    uint8_t *pBuf = reinterpret_cast<uint8_t*>(buffer);
    while (size)
    {
        if (m_bIsEOF)
        {
            // If EOF has been signalled or we already have some data,
            // we mustn't block, so tentatively attempt to tryAcquire until
            // it fails.
            if (!m_BufLen.tryAcquire())
            {
                // No data left and currently EOF - END.
                return n;
            }

            // Otherwise, we may have an un-acked EOF.
            /// \note There is a possibility that the pipe is actually full here
            ///       and that, should this return zero, we'll lose the data in
            ///       the pipe...
            if (m_Front == m_Back)
            {
                return n;
            }
        }
        else
        {
            // EOF not signalled, so do a blocking wait on data, if we can.
            if(bCanBlock)
                m_BufLen.acquire();
            else
            {
                if(!m_BufLen.tryAcquire())
                {
                    return n;
                }
            }

            // Is EOF signalled (is this why we were woken?)
            if (m_bIsEOF && (m_Front == m_Back))
            {
                // Yes, the next call will probably return zero.
                return n;
            }
        }

        // Otherwise, there is data available. If m_Front == m_Back, the
        // pipe is currently *full*.
        pBuf[n++] = m_Buffer[m_Front];
        m_Front = (m_Front+1) % PIPE_BUF_MAX;
        m_BufAvailable.release();
        size --;
    }
    return n;
}

uint64_t Pipe::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    uint64_t n = 0;
    uint8_t *pBuf = reinterpret_cast<uint8_t*>(buffer);
    while (size)
    {
        m_BufAvailable.acquire();
        m_Buffer[m_Back] = pBuf[n++];
        m_Back = (m_Back+1) % PIPE_BUF_MAX;
        m_BufLen.release();
        size --;
    }

    dataChanged();
    return n;
}

void Pipe::increaseRefCount(bool bIsWriter)
{
    if (bIsWriter)
    {
        if (m_bIsEOF)
        {
            // Start the pipe again.
            m_bIsEOF = false;
            while (m_BufLen.tryAcquire()) ;
            m_Front = m_Back = 0;
        }
        m_nWriters++;
    }
    else
    {
        m_nReaders++;
    }
}

void Pipe::decreaseRefCount(bool bIsWriter)
{
    // Make sure only one thread decreases the refcount at a time. This is
    // important as we add ourselves to the ZombieQueue if the refcount ticks
    // to zero. Getting pre-empted by another thread that also decreases the
    // refcount between the decrement and the check for zero may mean the pipe
    // is added to the ZombieQueue twice, which causes a double free.
    bool bDataChanged = false;
    {
        LockGuard<Mutex> guard(m_Lock);

        if (m_nReaders == 0 && m_nWriters == 0)
        {
            // Refcount is already zero - don't decrement! (also, bad.)
            ERROR("Pipe: decreasing refcount when refcount is already zero.");
            return;
        }

        if (bIsWriter)
        {
            m_nWriters --;
            if (m_nWriters == 0)
            {
                m_bIsEOF = true;
                if (m_Front == m_Back)
                {
                    // No data available - post the m_BufLen semaphore to wake any readers up.
                    m_BufLen.release();
                }

                bDataChanged = true;
            }
        }
        else
            m_nReaders --;

        if (m_nReaders == 0 && m_nWriters == 0)
        {
            // If we're anonymous, die completely.
            if (m_bIsAnonymous)
            {
                size_t pid = Processor::information().getCurrentThread()->getParent()->getId();
                ZombieQueue::instance().addObject(new ZombiePipe(this));
                bDataChanged = false;
            }
        }
    }

    if (bDataChanged)
    {
        dataChanged();
    }
}
