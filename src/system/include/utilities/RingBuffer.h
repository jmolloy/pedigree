/*
 * Copyright (c) 2013 Matthew Iselin
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

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <processor/types.h>
#include <process/Semaphore.h>

namespace RingBufferWait
{
    enum WaitType
    {
        Reading,
        Writing
    };
}

/**
 * \brief Utility class to provide a ring buffer.
 *
 * Using this class provides safety in accessing the ring buffer as well as
 * the ability to check (with and without blocking) whether the buffer can
 * be read or written to at this time.
 *
 * The idea of the waitFor function is to provide a way for applications
 * desiring integration with a select()-style interface to block until the
 * condition is met.
 *
 * \todo implement something similar to File::monitor, which fires an event
 *       when any activity takes place in the File.
 */
template <class T>
class RingBuffer
{
    public:
        RingBuffer(); // Not implemented, use RingBuffer(size_t)

        /// Constructor - pass in the desired size of the ring buffer.
        RingBuffer(size_t ringSize) :
            m_RingSize(ringSize), m_ReadSem(0), m_WriteSem(ringSize),
            m_Reader(0), m_Writer(0), m_Ring(0)
        {
            m_Ring = new T[ringSize];
        }

        /// Destructor - destroys the ring; ensure nothing is calling waitFor.
        virtual ~RingBuffer()
        {
            delete [] m_Ring;
        }

        /// write - write a byte to the ring buffer.
        void write(T obj)
        {
            m_WriteSem.acquire();
            m_Ring[m_Writer++] = obj;

            m_Writer %= m_RingSize;
            m_ReadSem.release();
        }

        /// write - write the given number of objects to the ring buffer.
        size_t write(T *obj, size_t n)
        {
            if(n > m_RingSize)
                n = m_RingSize;

            m_WriteSem.acquire(n);
            size_t totalCopied = n;
            if((m_Writer + n) > m_RingSize)
            {
                size_t copySize = m_RingSize - m_Writer;
                memcpy(&m_Ring[m_Writer], obj, copySize * sizeof(T));
                n -= copySize;
                obj += copySize;
                m_Writer = 0;
            }

            memcpy(&m_Ring[m_Writer], obj, n * sizeof(T));
            m_Writer += n;
            m_ReadSem.release(totalCopied);

            return totalCopied;
        }

        /// read - read a byte from the ring buffer.
        T read()
        {
            m_ReadSem.acquire();
            T ret = m_Ring[m_Reader++];

            m_Reader %= m_RingSize;
            m_WriteSem.release();

            return ret;
        }

        /// read - read up to the given number of objects from the ring buffer
        size_t read(T *out, size_t n)
        {
            if(n > m_RingSize)
                n = m_RingSize;

            if(n > m_ReadSem.getValue())
                n = m_ReadSem.getValue();

            m_ReadSem.acquire(n);
            size_t totalCopied = n;
            if((m_Reader + n) > m_RingSize)
            {
                size_t copySize = m_RingSize - m_Reader;
                memcpy(out, &m_Ring[m_Reader], copySize * sizeof(T));
                n -= copySize;
                out += copySize;
                m_Reader = 0;
            }

            memcpy(out, &m_Ring[m_Reader], n * sizeof(T));
            m_Reader += n;
            m_WriteSem.release(totalCopied);

            return totalCopied;
        }

        /// dataReady - is data ready for reading from the ring buffer?
        bool dataReady()
        {
            return m_ReadSem.getValue() > 0;
        }

        /// canWrite - is it possible to write to the ring buffer without blocking?
        bool canWrite()
        {
            return m_WriteSem.getValue() > 0;
        }

        /// waitFor - block until the given condition is true (readable/writeable)
        void waitFor(RingBufferWait::WaitType wait)
        {
            Semaphore *pSem = &m_ReadSem;
            if(wait == RingBufferWait::Writing)
            {
                pSem = &m_WriteSem;
            }

            pSem->acquire();
            pSem->release();
        }

        private:
        size_t m_RingSize;

        Semaphore m_ReadSem;
        Semaphore m_WriteSem;

        size_t m_Reader;
        size_t m_Writer;

        T *m_Ring;
};


#endif

