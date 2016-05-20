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

#ifndef MACHINE_TCPMISC_H
#define MACHINE_TCPMISC_H

#include <utilities/Tree.h>
#include <utilities/Iterator.h>
#include <utilities/List.h>
#include "Endpoint.h"

#include <Log.h>

#ifdef THREADS
#include <LockGuard.h>
#include <process/Mutex.h>
#endif

/** A TCP "Buffer" (also known as a stream) */
class TcpBuffer
{
  public:

    TcpBuffer() :
      m_BufferSize(0), m_DataSize(0)
#ifdef THREADS
      , m_Lock(false)
#endif
    {
      setSize(32768);
    }
    virtual ~TcpBuffer()
    {
      setSize(0);
    }

    /** Writes data to the buffer */
    size_t write(uintptr_t buffer, size_t nBytes);
    /** Reads data from the buffer */
    size_t read(uintptr_t buffer, size_t nBytes, bool bDoNotMove = false);

    /** Gets the number of bytes of data in the buffer */
    inline size_t getDataSize() const
    {
        return m_DataSize;
    }
    /** Gets the size of the buffer */
    inline size_t getSize() const
    {
        return m_BufferSize;
    }
    /** Sets the size of the buffer (ie, resize) */
    void setSize(size_t newBufferSize);

    /** Retrieves the number of bytes remaining in the buffer */
    inline size_t getRemainingSize() const
    {
        return m_BufferSize - m_DataSize;
    }

  private:
    /** Current buffer size */
    size_t m_BufferSize;

    /** Data size */
    size_t m_DataSize;

    static const size_t m_SegmentBufferSize = 65536;

    struct Segment
    {
      Segment() : buffer(), reader(0), size(0)
      {
      }

      /// Buffer in which the segment is stored.
      uint8_t buffer[m_SegmentBufferSize];
      /// Read offset so far.
      size_t reader;
      /// Number of bytes in this segment so far.
      size_t size;
    };

    List<Segment *> m_Segments;

    /** Create a new segment containing the given data. */
    void newSegment(uintptr_t buffer, size_t size);

    /** Read data from the given segment. */
    size_t readSegment(Segment *pSegment, uintptr_t target, size_t size, bool bUpdate = true);

#ifdef THREADS
    /** Buffer lock */
    Mutex m_Lock;
#endif
};

/** Connection state block handle */
struct StateBlockHandle
{
  StateBlockHandle() :
    localPort(0), remotePort(0), remoteHost(), listen(false)
  {}

  uint16_t localPort;
  uint16_t remotePort;
  Endpoint::RemoteEndpoint remoteHost;

  bool listen;

  bool operator == (StateBlockHandle a)
  {
    if(a.listen) // Require the client to want listen sockets only
    {
      if(listen)
        return (localPort == a.localPort);
    }
    else
    {
      // NOTICE_NOLOCK("Operator == [" << localPort << ", " << a.localPort << "] [" << remotePort << ", " << a.remotePort << "]" << " [" << remoteHost.ip.toString() << ", " << a.remoteHost.ip.toString() << "]");
      return ((localPort == a.localPort) && (remoteHost.ip == a.remoteHost.ip) && (remotePort == a.remotePort));
    }
    return false;
  }

  bool operator > (StateBlockHandle a)
  {
    return true;
    if(listen && a.listen)
      return (localPort > a.localPort);
    else
    {
      //NOTICE("Operator > : [" << localPort << ", " << a.localPort << "] [" << remotePort << ", " << a.remotePort << "]");
      if(localPort)
        return ((localPort > a.localPort) && (remotePort > a.remotePort));
      else
        return (remotePort > a.remotePort);
    }
  }
};

#endif
