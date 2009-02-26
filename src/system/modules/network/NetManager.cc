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

NetManager NetManager::m_Instance;

File NetManager::newEndpoint(int protocol)
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
  else
  {
    ERROR("NetManager::getEndpoint called with unknown protocol");
  }
  
  if(p)
  {
    size_t n = m_Endpoints.count();
    m_Endpoints.pushBack(p);
    return File(String("socket"), 0, 0, 0, n + 0xab000000, false, false, this, protocol);
  }
  else
    return File();
}

void NetManager::removeEndpoint(File f)
{
  if(!isEndpoint(f))
    return;
  
  Endpoint* e = getEndpoint(f);
  if(!e)
    return;
  e->close();
  
  size_t removeIndex = f.getInode() & 0x00FFFFFF, i = 0;
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
  
  if(f.getSize() == NETMAN_PROTO_UDP)
    UdpManager::instance().returnEndpoint(e);
  else if(f.getSize() == NETMAN_PROTO_TCP)
    TcpManager::instance().returnEndpoint(e);
}

bool NetManager::isEndpoint(File f)
{
  /// \todo We can actually check the filename too - if it's "socket" & the flags are set we know it's a socket...
  return ((f.getInode() & 0xFF000000) == 0xab000000);
}

Endpoint* NetManager::getEndpoint(File f)
{
  if(!isEndpoint(f))
    return 0;
  size_t indx = f.getInode() & 0x00FFFFFF;
  if(indx < m_Endpoints.count())
    return m_Endpoints[indx];
  else
    return 0;
}

File NetManager::accept(File f)
{
  if(!isEndpoint(f))
    return File();
  
  Endpoint* server = getEndpoint(f);
  if(server)
  {
    Endpoint* client = server->accept();
    if(client)
    {
      size_t n = m_Endpoints.count();
      m_Endpoints.pushBack(client);
      return File(String("socket"), 0, 0, 0, n + 0xab000000, false, false, this, f.getSize());
    }
  }
  return File();
}
