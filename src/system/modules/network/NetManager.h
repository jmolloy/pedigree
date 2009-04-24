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

#ifndef NETMANAGER_H
#define NETMANAGER_H

#include <vfs/VFS.h>
#include <vfs/Filesystem.h>
#include <utilities/RequestQueue.h>
#include <utilities/Vector.h>
#include <process/Scheduler.h>

#include <network/Endpoint.h>
#include <network/Tcp.h>

#define NETMAN_TYPE_UDP    1
#define NETMAN_TYPE_TCP    2
#define NETMAN_TYPE_RAW    3
#define NETMAN_TYPE_UDP6   10
#define NETMAN_TYPE_TCP6   11

#define IN_PROTOCOLS_DEFINED
enum Protocol
{
  IPPROTO_IP = 0,
  IPPROTO_IPV6,
  IPPROTO_ICMP,
  IPPROTO_RAW,
  IPPROTO_TCP,
  IPPROTO_UDP
};

/** File subclass for sockets */
class Socket : public File
{
  private:
    /** Copy constructors are hidden - (mostly) unimplemented (or invalid)! */
    Socket(const File &file);
    File& operator =(const File&);
    
    // Endpoints are not able to be copied
    Socket(const Socket &file) : m_Endpoint(0), m_Protocol(0)
    {
      ERROR("Socket copy constructor called");
    };
    Socket& operator =(const Socket &file)
    {
      ERROR("Socket copy constructor called");
      return *this;
    }
    
  public:
    Socket(int proto, Endpoint *p, Filesystem *pFs) :
      File(String("socket"), 0, 0, 0, 0, pFs, 0, 0), m_Endpoint(p), m_Protocol(proto)
    {};
    virtual ~Socket()
    {};
    
    inline Endpoint *getEndpoint()
    {
      return m_Endpoint;
    }
    
    inline int getProtocol()
    {
      return m_Protocol;
    }
    
    /** Similar to POSIX's select() function */
    virtual int select(bool bWriting = false, int timeout = 0)
    {
        NOTICE("Socket::select");
        
        // Basically, this busy waits for data; the last thing you want to do is
        // cancel execution of dataReady if it's blocking (with a timeout) as it
        // won't be able to free the resources it uses or unregister the Timer it
        // uses for the timeout.
        if(!bWriting)
        {
            do
            {
                int state = m_Endpoint->state();
                if(m_Protocol == NETMAN_TYPE_TCP)
                {
                    if(state == Tcp::ESTABLISHED)
                    {
                        if(m_Endpoint->dataReady(false))
                            return 1;
                    }
                    else
                        return 1; // not established, allow EOF from recv()
                }
                else if(m_Endpoint->dataReady(false))
                    return 1; // non-TCP sockets just need to check for data

                Scheduler::instance().yield();
            }
            while(timeout != 0);
        }
        else
        {
            if(m_Protocol == NETMAN_TYPE_TCP)
            {
                do
                {
                    int state = m_Endpoint->state();
                    if(state >= Tcp::ESTABLISHED && state < Tcp::CLOSE_WAIT)
                        return 1;
                    
                    Scheduler::instance().yield();
                }
                while(timeout != 0);
            }
            else
                return 1;
        }
        
        return 0;
    }

    virtual void decreaseRefCount(bool bIsWriter);
    
  private:
  
    Endpoint *m_Endpoint;
    int m_Protocol;
};

/** Provides an interface to Endpoints for applications */
class NetManager : public Filesystem
{
public:
  NetManager() : m_Endpoints()
  {
    m_Endpoints.clear();
  };

  virtual ~NetManager()
  {
  };

  static NetManager &instance()
  {
    return m_Instance;
  };

  //
  // NetManager interface.
  //
  
  File* newEndpoint(int type, int protocol);
  
  bool isEndpoint(File* f);
  
  Endpoint* getEndpoint(File* f);
  
  void removeEndpoint(File* f);
  
  File* accept(File* f);
  
  uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
  uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);

  //
  // Filesystem interface.
  //

  virtual bool initialise(Disk *pDisk)
    {return false;}
  virtual File* getRoot()
  {return 0;}
  virtual String getVolumeLabel()
  {return String("netman");}
  virtual void truncate(File *pFile)
  {}
  virtual void fileAttributeChanged(File *pFile)
  {}
  virtual void cacheDirectoryContents(File *pFile)
  {}

protected:
  virtual bool createFile(File* parent, String filename, uint32_t mask)
  {return false;}
  virtual bool createDirectory(File* parent, String filename)
  {return false;}
  virtual bool createSymlink(File* parent, String filename, String value)
  {return false;}
  virtual bool remove(File* parent, File* file)
  {return false;}

private:

  Vector<Endpoint*> m_Endpoints;
  static NetManager m_Instance;
};

#endif
