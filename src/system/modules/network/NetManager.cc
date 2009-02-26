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
  Endpoint* p;
  if(protocol == NETMAN_PROTO_UDP)
  {
    IpAddress a;
    p = UdpManager::instance().getEndpoint(a, 0, 0);
  }
  else if(protocol == NETMAN_PROTO_TCP)
  {
    /// \todo Potential for multiple cards, that'll need to be handled
    p = TcpManager::instance().getEndpoint();
  }
  else
  {
    ERROR("NetManager::getEndpoint called with unknown protocol");
  }
  
  size_t n = m_Endpoints.count();
  m_Endpoints.pushBack(p);
  return File(String("socket"), 0, 0, 0, n + 0xab000000, false, false, this, protocol);
}

void NetManager::removeEndpoint(File f)
{
  size_t removeIndex = f.getInode() & 0x00FFFFFF;
  // find it and remove it
  
  Endpoint* e = getEndpoint(f);
  e->close(); // UDP endpoints don't implement close() so the default Endpoint::close() would be called, which is a stub
  
  // clean up the actual endpoint itself too...
}

bool NetManager::isEndpoint(File f)
{
  /// \todo We can actually check the filename too - if it's "socket" & the flags are set we know it's a socket...
  return ((f.getInode() & 0xFF000000) == 0xab000000);
}

Endpoint* NetManager::getEndpoint(File f)
{
  return m_Endpoints[f.getInode() & 0x00FFFFFF];
}

File NetManager::accept(File f)
{
  Endpoint* server = getEndpoint(f);
  
  Endpoint* client = server->accept();
  
  size_t n = m_Endpoints.count();
  m_Endpoints.pushBack(client);
  return File(String("socket"), 0, 0, 0, n + 0xab000000, false, false, this, f.getSize());
}
