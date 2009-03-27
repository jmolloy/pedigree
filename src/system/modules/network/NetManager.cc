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

#include "NetManager.h" 
#include <network/Endpoint.h>
#include <network/UdpManager.h>
#include <network/TcpManager.h>
#include <network/RawManager.h>

NetManager NetManager::m_Instance;

File* NetManager::newEndpoint(int protocol)
{
  Endpoint* p = 0;
  if(protocol == NETMAN_PROTO_UDP)
  {
    IpAddress a;
    p = UdpManager::instance().getEndpoint(a, 0, 0);
  }
  else if(protocol == NETMAN_PROTO_TCP)
  {
    p = TcpManager::instance().getEndpoint();
  }
  else if(protocol == NETMAN_PROTO_RAW)
  {
    p = RawManager::instance().getEndpoint();
  }
  else
  {
    ERROR("NetManager::getEndpoint called with unknown protocol");
  }
  
  if(p)
  {
    size_t n = m_Endpoints.count();
    m_Endpoints.pushBack(p);
    return new File(String("socket"), 0, 0, 0, n + 0xab000000, false, false, this, protocol);
  }
  else
    return VFS::invalidFile();
}

void NetManager::removeEndpoint(File* f)
{
  if(!isEndpoint(f))
    return;
  
  Endpoint* e = getEndpoint(f);
  if(!e)
    return;
  e->close();
  
  size_t removeIndex = f->getInode() & 0x00FFFFFF, i = 0;
  bool removed = false;
  for(Vector<Endpoint*>::Iterator it = m_Endpoints.begin(); it != m_Endpoints.end(); it++, i++)
  {
    if(i == removeIndex)
    {
      m_Endpoints.erase(it);
      removed = true;
      break;
    }
  }
  
  //if(f->getSize() == NETMAN_PROTO_UDP)
  //  UdpManager::instance().returnEndpoint(e);
  //else if(f->getSize() == NETMAN_PROTO_TCP)
  //  TcpManager::instance().returnEndpoint(e);

  if(f->getSize() == NETMAN_PROTO_RAW)
    RawManager::instance().returnEndpoint(e);
}

bool NetManager::isEndpoint(File* f)
{
  /// \todo We can actually check the filename too - if it's "socket" & the flags are set we know it's a socket...
  return ((f->getInode() & 0xFF000000) == 0xab000000);
}

Endpoint* NetManager::getEndpoint(File* f)
{
  if(!isEndpoint(f))
    return 0;
  size_t indx = f->getInode() & 0x00FFFFFF;
  if(indx < m_Endpoints.count())
    return m_Endpoints[indx];
  else
    return 0;
}

File* NetManager::accept(File* f)
{
  if(!isEndpoint(f))
    return VFS::invalidFile();
  
  Endpoint* server = getEndpoint(f);
  if(server)
  {
    Endpoint* client = server->accept();
    if(client)
    {
      size_t n = m_Endpoints.count();
      m_Endpoints.pushBack(client);
      return new File(String("socket"), 0, 0, 0, n + 0xab000000, false, false, this, f->getSize());
    }
  }
  return VFS::invalidFile();
}

uint64_t NetManager::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  NOTICE("NetManager::read");
  
  Endpoint* p = NetManager::instance().getEndpoint(pFile);
    
  int ret = 0;
  if(pFile->getSize() == NETMAN_PROTO_TCP)
  {
    /// \todo O_NONBLOCK should control the blocking nature of this call
    ret = p->recv(buffer, size, false, false);
  }
  else if(pFile->getSize() == NETMAN_PROTO_UDP)
  {
    /// \todo Actually, we only should read this data if it's from the IP specified
    ///       during connect - otherwise we fail (UDP should use sendto/recvfrom)
    ///       However, to do that we need to tell recv not to remove from the queue
    ///       and instead peek at the message (in other words, we need flags)
    Endpoint::RemoteEndpoint remoteHost;
    ret = p->recv(buffer, size, &remoteHost);
  }
  else if(pFile->getSize() == NETMAN_PROTO_RAW)
  {
    ret = p->recv(buffer, size, 0);
  }
  
  return ret;
}

uint64_t NetManager::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  NOTICE("NetManager::write");
  
  Endpoint* p = NetManager::instance().getEndpoint(pFile);
  
  bool success = false;
  if(pFile->getSize() == NETMAN_PROTO_TCP)
  {
    success = p->send(size, buffer);
  }
  else if(pFile->getSize() == NETMAN_PROTO_UDP)
  {
    // special handling - need to check for a remote host
    IpAddress remoteIp = p->getRemoteIp();
    if(remoteIp.getIp() != 0)
    {
      Endpoint::RemoteEndpoint remoteHost;
      remoteHost.remotePort = p->getRemotePort();
      remoteHost.ip = remoteIp;
      success = p->send(size, buffer, remoteHost, false, NetworkStack::instance().getDevice(0));
    }
  }
  else if(pFile->getSize() == NETMAN_PROTO_RAW)
  {
    Endpoint::RemoteEndpoint remoteHost;
    p->send(size, buffer, remoteHost, false, NetworkStack::instance().getDevice(0));
  }
  
  return success ? size : 0;
}
