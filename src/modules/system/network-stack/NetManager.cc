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
#include <network-stack/Endpoint.h>
#include <network-stack/UdpManager.h>
#include <network-stack/TcpManager.h>
#include <network-stack/RawManager.h>
#include <network-stack/RoutingTable.h>
#include <network-stack/ConnectionBasedEndpoint.h>
#include <utilities/ZombieQueue.h>

NetManager NetManager::m_Instance;

class ZombieSocket : public ZombieObject
{
    public:
        ZombieSocket(Socket *pSocket) : m_pSocket(pSocket)
        {
        }
        virtual ~ZombieSocket()
        {
            delete m_pSocket;
        }
    private:
        Socket *m_pSocket;
};

uint64_t Socket::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return reinterpret_cast<NetManager*>(m_pFilesystem)->read(this, location, size, buffer, bCanBlock);
}
uint64_t Socket::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return reinterpret_cast<NetManager*>(m_pFilesystem)->write(this, location, size, buffer, bCanBlock);
}

void Socket::decreaseRefCount(bool bIsWriter)
{
    if (bIsWriter)
        m_nWriters --;
    else
        m_nReaders --;

    m_Endpoint->RemoveSocket(this);

    if (m_nReaders == 0 && m_nWriters == 0)
    {
        if (m_Endpoint)
        {
            m_Endpoint->shutdown(Endpoint::ShutBoth);
            if(m_Endpoint->getType() == Endpoint::ConnectionBased)
            {
                ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(m_Endpoint);
                ce->close();
            }
            m_Endpoint->getManager()->returnEndpoint(m_Endpoint);
        }
        ZombieQueue::instance().addObject(new ZombieSocket(this));
    }
}

File* NetManager::newEndpoint(int type, int protocol)
{
    Endpoint* p = 0;
    if (type == NETMAN_TYPE_UDP)
    {
        IpAddress a;
        p = UdpManager::instance().getEndpoint(a, 0, 0);
    }
    else if (type == NETMAN_TYPE_TCP)
    {
        p = TcpManager::instance().getEndpoint();
    }
    else if (type == NETMAN_TYPE_RAW)
    {
        p = RawManager::instance().getEndpoint(protocol);
    }
    else
    {
        ERROR("NetManager::getEndpoint called with unknown protocol");
    }

    if (p)
    {
        File *ret = new Socket(type, p, this);
        ret->increaseRefCount(false);
        p->AddSocket(static_cast<Socket*>(ret));
        return ret;
    }
    else
        return 0;
}

void NetManager::removeEndpoint(File* f)
{
    ERROR("Old-style call (removeEndpoint)");
    return;

    if (!isEndpoint(f))
        return;

    Endpoint* e = getEndpoint(f);
    if (!e)
        return;

    e->shutdown(Endpoint::ShutBoth);
    if(e->getType() == Endpoint::ConnectionBased)
    {
        ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(e);
        ce->close();
    }

    size_t removeIndex = f->getInode() & 0x00FFFFFF, i = 0;
    bool removed = false;
    for (Vector<Endpoint*>::Iterator it = m_Endpoints.begin(); it != m_Endpoints.end(); it++, i++)
    {
        if (i == removeIndex)
        {
            m_Endpoints.erase(it);
            removed = true;
            break;
        }
    }
}

bool NetManager::isEndpoint(File* f)
{
    /// \todo We can actually check the filename too - if it's "socket" & the flags are set we know it's a socket...
    return f && ((f->getInode() & 0xFF000000) == 0xab000000);
}

Endpoint* NetManager::getEndpoint(File* f)
{
    ERROR("Old-style call (getEndpoint)");
    return 0;

    if (!isEndpoint(f))
        return 0;
    size_t indx = f->getInode() & 0x00FFFFFF;
    if (indx < m_Endpoints.count())
        return m_Endpoints[indx];
    else
        return 0;
}

File* NetManager::accept(File* f)
{
    // We can pretty safely assume that this is a valid call, as the only File objects
    // defined in the context of the NetManager are Sockets
    Socket *sock = static_cast<Socket *>(f);

    Endpoint* server = sock->getEndpoint();
    if(server && server->getType() == Endpoint::ConnectionBased)
    {
        ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(server);
        Endpoint* client = ce->accept();

        File *ret = new Socket(sock->getProtocol(), client, this);
        ret->increaseRefCount(false);
        return ret;
    }
    return 0;
}

uint64_t NetManager::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    // We can pretty safely assume that this is a valid call, as the only File objects
    // defined in the context of the NetManager are Sockets
    Socket *sock = static_cast<Socket *>(pFile);

    Endpoint* p = sock->getEndpoint();

    int ret = 0;
    if (p->getType() == Endpoint::Connectionless)
    {
        ConnectionlessEndpoint *ce = static_cast<ConnectionlessEndpoint *>(p);

        /// \note UDP specific todo
        /// \todo Actually, we only should read this data if it's from the IP specified
        ///       during connect - otherwise we fail (UDP should use sendto/recvfrom)
        ///       However, to do that we need to tell recv not to remove from the queue
        ///       and instead peek at the message (in other words, we need flags)
        Endpoint::RemoteEndpoint remoteHost;
        memset(&remoteHost, 0, sizeof(Endpoint::RemoteEndpoint));
        ret = ce->recv(buffer, size, bCanBlock, &remoteHost);
    }
    else
    {
        ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(p);
        ret = ce->recv(buffer, size, bCanBlock, false);
    }

    return ret;
}

uint64_t NetManager::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    // We can pretty safely assume that this is a valid call, as the only File objects
    // defined in the context of the NetManager are Sockets
    Socket *sock = static_cast<Socket *>(pFile);

    Endpoint* p = sock->getEndpoint();

    if (p->getType() == Endpoint::Connectionless)
    {
        ConnectionlessEndpoint *ce = static_cast<ConnectionlessEndpoint *>(p);

        Network *pCard = 0;
        Endpoint::RemoteEndpoint remoteHost;
        IpAddress remoteIp = p->getRemoteIp();
        if(sock->getProtocol() == NETMAN_TYPE_UDP)
        {
            if (remoteIp.getIp() != 0)
            {
                remoteHost.remotePort = p->getRemotePort();
                remoteHost.ip = remoteIp;
            }
            else
                return 0; // 0 != valid IP
        }

        return ce->send(size, buffer, remoteHost, false);
    }
    else
    {
        ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(p);
        return ce->send(size, buffer);
    }

    return 0;
}

