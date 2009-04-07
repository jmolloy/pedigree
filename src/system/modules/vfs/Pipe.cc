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

#include "Pipe.h"
#include "Filesystem.h"

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

uint64_t Pipe::read(uint64_t location, uint64_t size, uintptr_t buffer)
{
  uint64_t n = 0;
  uint8_t *pBuf = reinterpret_cast<uint8_t*>(buffer);
  while (size)
  {
    if (m_bIsEOF || n > 0)
    {
      // If EOF has been signalled or we already have some data, we mustn't block, so tentatively attempt to tryAcquire until it fails.
      if (!m_BufLen.tryAcquire())
      {
        // No data left and/or EOF given - END.
        return n;
      }
      pBuf[n++] = m_Buffer[m_Front];
      m_Front = (m_Front+1) % PIPE_BUF_MAX;
      m_BufAvailable.release();
      size --;
    }
    else
    {
      // EOF not signalled, so do a blocking wait on data.
      m_BufLen.acquire();

      // However, *here*, we must check if EOF is now signalled. When EOF comes in and there is no data, m_BufLen will be posted to
      // wake us up. We need to check if there was data available.
      if (m_Front == m_Back)
      {
        // We were woken with no data available. This means we must return now.
        NOTICE("Pipe: woken with no data, assuming EOF.");
        return 0;
      }

      pBuf[n++] = m_Buffer[m_Front];
      m_Front = (m_Front+1) % PIPE_BUF_MAX;
      m_BufAvailable.release();
      size --;
    }
  }
  return n;
}

uint64_t Pipe::write(uint64_t location, uint64_t size, uintptr_t buffer)
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
  return n;
}

void Pipe::increaseRefCount(bool bIsWriter)
{
  NOTICE("Pipe: increaseRefCount (" << bIsWriter << "): " << m_nWriters << " writers and " << m_nReaders << " readers currently.");
  if (bIsWriter)
  {
    if (m_bIsEOF)
    {
      NOTICE("Pipe: reviving pipe because a writer joined again.");
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
  NOTICE("Pipe: decreaseRefCount (" << bIsWriter << "): " << m_nWriters << " writers and " << m_nReaders << " readers currently.");
  if (bIsWriter)
  {
    m_nWriters --;
    if (m_nWriters == 0)
    {
      NOTICE("Pipe: Setting EOF, because no writers left.");
      m_bIsEOF = true;
      if (m_Front == m_Back)
      {
        // No data available - post the m_BufLen semaphore to wake any readers up.
        m_BufLen.release();
      }
    }
  }
  else
    m_nReaders --;

  if (m_nReaders == 0 && m_nWriters == 0)
  {
    // If we're anonymous, die completely.
    if (m_bIsAnonymous)
    {
      NOTICE("Pipe: destroying.");
      delete this;
    }
  }
}
