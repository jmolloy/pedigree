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

#include <syscallError.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <process/Process.h>
#include <utilities/Tree.h>
#include <vfs/File.h>
#include <vfs/VFS.h>
#include <machine/Network.h>
#include <network-stack/NetManager.h>
#include <network-stack/NetworkStack.h>
#include <network-stack/RoutingTable.h>
#include <network-stack/Dns.h>
#include <network-stack/Tcp.h>

#include <Subsystem.h>
#include <PosixSubsystem.h>

#include "file-syscalls.h"
#include "net-syscalls.h"

#include "newlib.h"

int posix_socket(int domain, int type, int protocol)
{
    N_NOTICE("socket(" << domain << ", " << type << ", " << protocol << ")");

    // Lookup this process.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return -1;
    }

    size_t fd = pSubsystem->getFd();

    File* file = 0;
    bool valid = true;
    if (domain == AF_INET)
    {
        if ((type == SOCK_DGRAM && protocol == 0) || (type == SOCK_DGRAM && protocol == IPPROTO_UDP))
            file = NetManager::instance().newEndpoint(NETMAN_TYPE_UDP, protocol);
        else if ((type == SOCK_STREAM && protocol == 0) || (type == SOCK_STREAM && protocol == IPPROTO_TCP))
            file = NetManager::instance().newEndpoint(NETMAN_TYPE_TCP, protocol);
        else if (type == SOCK_RAW)
        {
            file = NetManager::instance().newEndpoint(NETMAN_TYPE_RAW, protocol);
        }
        else
        {
            valid = false;
        }
    }
    else if (domain == PF_SOCKET)
    {
        // raw wire-level sockets
        file = NetManager::instance().newEndpoint(NETMAN_TYPE_RAW, 0xff);
    }
    else
    {
        WARNING("domain = " << domain << " - not known!");
        valid = false;
    }

    if (!valid)
        return -1;

    FileDescriptor *f = new FileDescriptor;
    f->file = file;
    f->offset = 0;
    f->fd = fd;
    pSubsystem->addFileDescriptor(fd, f);

    return static_cast<int> (fd);
}

int posix_connect(int sock, struct sockaddr* address, size_t addrlen)
{
    N_NOTICE("posix_connect(" << sock << ", " << reinterpret_cast<uintptr_t>(address) << ", " << addrlen << ")");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
    if (!f || !f->file)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }
    Socket *s = static_cast<Socket *>(f->file);

    Endpoint* p = s->getEndpoint();
    if(p->getType() == Endpoint::ConnectionBased)
    {
        ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(p);
        int endpointState = ce->state();
        if (endpointState < Tcp::CLOSED)
        {
            if (endpointState < Tcp::ESTABLISHED)
            {
                // EALREADY - connection attempt in progress
                SYSCALL_ERROR(Already);
                return -1;
            }
            else
            {
                // EISCONN - already connected
                SYSCALL_ERROR(IsConnected);
                return -1;
            }
        }
    }

    bool blocking = !((f->flflags & O_NONBLOCK) == O_NONBLOCK);

    Endpoint::RemoteEndpoint remoteHost;

    /// \bug Assuming _in here...
    struct sockaddr_in* address_in = reinterpret_cast<struct sockaddr_in*>(address);
    IpAddress dest(address_in->sin_addr.s_addr);
    Network *iface = RoutingTable::instance().DetermineRoute(&dest);
    if(!iface || !iface->isConnected())
    {
        // Can't use this interface...
        return false;
    }

    /// \todo Other protocols
    bool success = false;
    if (s->getProtocol() == NETMAN_TYPE_TCP)
    {
        ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(p);

        struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
        remoteHost.remotePort = BIG_TO_HOST16(sin->sin_port);
        remoteHost.ip.setIp(sin->sin_addr.s_addr);

        success = ce->connect(remoteHost, blocking);

        if (!blocking)
        {
            SYSCALL_ERROR(InProgress);
            return -1;
        }
    }
    else if (s->getProtocol() == NETMAN_TYPE_UDP)
    {
        // connect on a UDP socket sets a remote address and port for send/recv
        // to send to multiple addresses and receive from multiple clients
        // sendto and recvfrom must be used

        ConnectionlessEndpoint *ce = static_cast<ConnectionlessEndpoint *>(p);

        struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
        remoteHost.remotePort = BIG_TO_HOST16(sin->sin_port);
        remoteHost.ip.setIp(sin->sin_addr.s_addr);

        ce->setRemotePort(remoteHost.remotePort);
        ce->setRemoteIp(remoteHost.ip);
        success = true;
    }
    else if (s->getProtocol() == NETMAN_TYPE_RAW)
    {
        success = true;
    }

    NOTICE("posix_connect returns");

    return success ? 0 : -1;
}

ssize_t posix_send(int sock, const void* buff, size_t bufflen, int flags)
{
    N_NOTICE("posix_send");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
    if (!f || !f->file)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }
    Socket *s = static_cast<Socket *>(f->file);

    Endpoint* p = s->getEndpoint();

    bool success = false;
    if (s->getProtocol() == NETMAN_TYPE_TCP)
    {
        ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(p);
        return ce->send(bufflen, reinterpret_cast<uintptr_t>(buff));
    }
    else if (s->getProtocol() == NETMAN_TYPE_UDP)
    {
        ConnectionlessEndpoint *ce = static_cast<ConnectionlessEndpoint *>(p);

        // special handling - need to check for a remote host
        IpAddress remoteIp = p->getRemoteIp();
        if (remoteIp.getIp() != 0)
        {
            Endpoint::RemoteEndpoint remoteHost;
            remoteHost.remotePort = p->getRemotePort();
            remoteHost.ip = remoteIp;
            ce->send(bufflen, reinterpret_cast<uintptr_t>(buff), remoteHost, false);
        }
    }

    return success ? bufflen : -1;
}

struct special_send_recv_data
{
    int sock;
    void* buff;
    size_t bufflen;
    int flags;
    struct sockaddr* remote_addr;
    socklen_t* addrlen;
} __attribute__((packed));

ssize_t posix_sendto(void* callInfo)
{
    N_NOTICE("posix_sendto");

    /// \todo Ugly - constructor would be nicer.
    special_send_recv_data* tmp = reinterpret_cast<special_send_recv_data*>(callInfo);
    int sock = tmp->sock;
    const void* buff = tmp->buff;
    size_t bufflen = tmp->bufflen;
    //int flags = tmp->flags;
    const sockaddr* address = tmp->remote_addr;
    //size_t* addrlen = tmp->addrlen;

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
    if (!f || !f->file)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }
    Socket *s = static_cast<Socket *>(f->file);

    Endpoint* p = s->getEndpoint();

    if (s->getProtocol() == NETMAN_TYPE_TCP)
    {
        /// \todo I need to write a sendto for TcpManager and TcpEndpoint
        ///       which probably means UDP gets a free sendto as well.
        ///       Until then, this is NOT valid according to the standards.
        ERROR("TCP sendto called, but not implemented properly!");
        ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(p);
        return ce->send(bufflen, reinterpret_cast<uintptr_t>(buff));
    }
    else if (s->getProtocol() == NETMAN_TYPE_UDP || s->getProtocol() == NETMAN_TYPE_RAW)
    {
        ConnectionlessEndpoint *ce = static_cast<ConnectionlessEndpoint *>(p);

        Endpoint::RemoteEndpoint remoteHost;
        const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(address);
        remoteHost.remotePort = BIG_TO_HOST16(sin->sin_port);
        remoteHost.ip.setIp(sin->sin_addr.s_addr);
        return ce->send(bufflen, reinterpret_cast<uintptr_t>(buff), remoteHost, false);
    }

    return -1;
}

ssize_t posix_recv(int sock, void* buff, size_t bufflen, int flags)
{
    N_NOTICE("posix_recv");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
    if (!f || !f->file)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }
    Socket *s = static_cast<Socket *>(f->file);

    Endpoint* p = s->getEndpoint();

    bool blocking = !((f->flflags & O_NONBLOCK) == O_NONBLOCK);

    int ret = -1;
    if (s->getProtocol() == NETMAN_TYPE_TCP)
    {
        ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(p);
        ret = ce->recv(reinterpret_cast<uintptr_t>(buff), bufflen, blocking, flags & MSG_PEEK);
    }
    else if (s->getProtocol() == NETMAN_TYPE_UDP)
    {
        /// \todo Actually, we only should read this data if it's from the IP specified
        ///       during connect - otherwise we fail (UDP should use sendto/recvfrom)
        ///       However, to do that we need to tell recv not to remove from the queue
        ///       and instead peek at the message (in other words, we need flags)
        ConnectionlessEndpoint *ce = static_cast<ConnectionlessEndpoint *>(p);
        Endpoint::RemoteEndpoint remoteHost;
        ret = ce->recv(reinterpret_cast<uintptr_t>(buff), bufflen, blocking, &remoteHost);
    }
    return ret;
}

ssize_t posix_recvfrom(void* callInfo)
{
    N_NOTICE("posix_recvfrom");

    /// \todo Ugly - constructor would be nicer.
    special_send_recv_data* tmp = reinterpret_cast<special_send_recv_data*>(callInfo);
    int sock = tmp->sock;
    void* buff = tmp->buff;
    size_t bufflen = tmp->bufflen;
    int flags = tmp->flags;
    sockaddr* address = tmp->remote_addr;
    size_t* addrlen = tmp->addrlen;

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
    if (!f || !f->file)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }
    Socket *s = static_cast<Socket *>(f->file);

    Endpoint* p = s->getEndpoint();

    bool blocking = !((f->flflags & O_NONBLOCK) == O_NONBLOCK);

    int ret = -1;
    if (s->getProtocol() == NETMAN_TYPE_TCP)
    {
        ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(p);

        ret = ce->recv(reinterpret_cast<uintptr_t>(buff), bufflen, blocking, flags & MSG_PEEK);

        struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
        sin->sin_port = HOST_TO_BIG16(p->getRemotePort());
        sin->sin_addr.s_addr = p->getRemoteIp().getIp();
        *addrlen = sizeof(struct sockaddr_in);
    }
    else if (s->getProtocol() == NETMAN_TYPE_UDP || s->getProtocol() == NETMAN_TYPE_RAW)
    {
        ConnectionlessEndpoint *ce = static_cast<ConnectionlessEndpoint *>(p);

        Endpoint::RemoteEndpoint remoteHost;
        ret = ce->recv(reinterpret_cast<uintptr_t>(buff), bufflen, blocking, &remoteHost);

        struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
        sin->sin_port = HOST_TO_BIG16(remoteHost.remotePort);
        sin->sin_addr.s_addr = remoteHost.ip.getIp();
        *addrlen = sizeof(struct sockaddr_in);
    }

    return ret;
}

int posix_bind(int sock, const struct sockaddr *address, size_t addrlen)
{
    N_NOTICE("posix_bind");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
    if (!f || !f->file)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }
    Socket *s = static_cast<Socket *>(f->file);

    Endpoint* p = s->getEndpoint();
    if (p)
    {
        int ret = -1;
        if (s->getProtocol() == NETMAN_TYPE_TCP || s->getProtocol() == NETMAN_TYPE_UDP)
        {
            const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(address);

            p->setLocalPort(BIG_TO_HOST16(sin->sin_port));

            ret = 0;
        }

        return ret;
    }
    else
        return -1;
}

int posix_listen(int sock, int backlog)
{
    N_NOTICE("posix_listen");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
    Socket *s = static_cast<Socket *>(f->file);
    if (!s)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }

    Endpoint* p = s->getEndpoint();
    if(p->getType() != Endpoint::ConnectionBased)
    {
        SYSCALL_ERROR(OperationNotSupported);
        return -1;
    }

    ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(p);

    if(s->getProtocol() == NETMAN_TYPE_TCP)
    {
        if((ce->state() != Tcp::CLOSED) && (ce->state() != Tcp::UNKNOWN))
        {
            ERROR("State was " << static_cast<int>(ce->state()) << ".");
            SYSCALL_ERROR(InvalidArgument);
            return -1;
        }
    }

    ce->listen();

    return 0;
}

int posix_accept(int sock, struct sockaddr* address, size_t* addrlen)
{
    N_NOTICE("posix_accept");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
    Socket *s1 = static_cast<Socket *>(f->file);
    if (!s1)
        return -1;

    Socket *s = static_cast<Socket *>(NetManager::instance().accept(s1));
    if (!s)
        return -1;

    // add into the descriptor table
    size_t fd = pSubsystem->getFd();

    Endpoint* e = s->getEndpoint(); // NetManager::instance().getEndpoint(f);

    if (address && addrlen)
    {
        if (s->getProtocol() == NETMAN_TYPE_TCP || s->getProtocol() == NETMAN_TYPE_UDP)
        {
            struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
            sin->sin_port = HOST_TO_BIG16(e->getRemotePort());
            sin->sin_addr.s_addr = e->getRemoteIp().getIp();

            *addrlen = sizeof(struct sockaddr_in);
        }
    }

    FileDescriptor *desc = new FileDescriptor(s, 0, fd, 0, O_RDWR);
    if (desc)
    {
        NOTICE("posix_accept: new socket is fd " << Dec << fd << Hex << ".");
        pSubsystem->addFileDescriptor(fd, desc);
    }

    return static_cast<int> (fd);
}

int posix_gethostbyaddr(const void* addr, size_t len, int type, void* ent)
{
    N_NOTICE("gethostbyaddr");
    return -1;
}

int posix_gethostbyname(const char* name, void* hostinfo, int offset)
{
    N_NOTICE("gethostbyname");

    // Sanity checks
    if (!hostinfo || !offset || !name)
        return -1;

    // Lookups can be done on the default interface
    Network* pCard = RoutingTable::instance().DefaultRoute();

    // Do the lookup
    Dns::HostInfo host;
    int lookup = Dns::instance().hostToIp(String(name), host, pCard);
    if(lookup == -1)
    {
        // None found on this interface, try the rest
        bool bFound = false;
        for(size_t i = 0; i < NetworkStack::instance().getNumDevices(); i++)
        {
            Network *pTmp = NetworkStack::instance().getDevice(i);
            if(pTmp != pCard)
            {
                if(pTmp->getStationInfo().nDnsServers)
                {
                    lookup = Dns::instance().hostToIp(String(name), host, pTmp);
                    if(lookup != -1)
                    {
                        bFound = true;
                        pCard = pTmp;
                        break;
                    }
                }
            }
        }

        if(!bFound)
        {
            N_NOTICE("gethostbyname: no IPs found for '" << name << "'.");
            return -1;
        }
    }

    // Grab the passed hostent
    struct hostent
    {
        char* h_name;
        char** h_aliases;
        int h_addrtype;
        int h_length;
        char** h_addr_list;
        char* h_addr;
    } *entry = reinterpret_cast<struct hostent*>(hostinfo);
    uintptr_t userBlock = reinterpret_cast<uintptr_t>(hostinfo) + sizeof(struct hostent);
    uintptr_t endBlock = (userBlock + offset) - sizeof(struct hostent);

    memset(hostinfo, 0, offset);

    // Copy the hostname
    entry->h_name = reinterpret_cast<char*>(userBlock);
    strncpy(entry->h_name, static_cast<const char*>(host.hostname), host.hostname.length());
    userBlock += host.hostname.length() + 1;

    // Make sure we don't overflow the buffer
    if (userBlock < endBlock)
    {
        // Create room for all the pointers to aliases
        uintptr_t aliasPointerBlock = userBlock;
        entry->h_aliases = reinterpret_cast<char**>(userBlock);
        userBlock += sizeof(char*) * (host.aliases.count() + 1);

        // Copy aliases across
        int nAlias = 0;
        for(List<String*>::Iterator it = host.aliases.begin();
            it != host.aliases.end() && userBlock < endBlock;
            it++)
        {
            if(!(*it))
                continue;

            entry->h_aliases[nAlias] = reinterpret_cast<char*>(userBlock);
            strncpy(entry->h_aliases[nAlias], static_cast<const char*>(*(*it)), (*it)->length());
            userBlock += (*it)->length() + 1;

            delete *it;
            nAlias++;
        }
    }
    else
        WARNING("gethostbyname: couldn't add aliases for '" << name << "', out of room in the userspace buffer");

    // Make sure we don't overflow the buffer
    if (userBlock < endBlock)
    {
        // Finally add the IP list
        char** addrList = reinterpret_cast<char**>(userBlock);
        userBlock += sizeof(char*) * (host.addresses.count() + 1); // Null terminated list

        // Load each IP into the buffer
        int nIp = 0;
        for(List<IpAddress*>::Iterator it = host.addresses.begin();
            it != host.addresses.end() && userBlock < endBlock;
            it++)
        {
            if(!(*it))
                continue;

            // Copy the IP across
            uint32_t ip = (*it)->getIp();
            char* ipBlock = reinterpret_cast<char*>(userBlock);
            memcpy(ipBlock, &ip, 4);

            // Add this to the array
            addrList[nIp++] = ipBlock;
            userBlock += 4;

            delete *it;
        }

        // Add the address list to the hostent structure
        entry->h_addrtype = AF_INET;
        entry->h_length = 4;
        entry->h_addr_list = addrList;
    }
    else
        WARNING("gethostbyname: couldn't add IP addresses for '" << name << "', out of room in the userspace buffer");

    return 0;
}

int posix_shutdown(int socket, int how)
{
    N_NOTICE("posix_shutdown");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(socket);
    Socket *s = static_cast<Socket *>(f->file);
    if (!s)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }

    Endpoint *e = s->getEndpoint();
    Endpoint::ShutdownType howType;
    if(how == SHUT_RD)
        howType = Endpoint::ShutReceiving;
    else if(how == SHUT_WR)
        howType = Endpoint::ShutSending;
    else if(how == SHUT_RDWR)
        howType = Endpoint::ShutBoth;
    else
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    if(e->shutdown(howType))
        return 0;
    else
        return -1;
}

int posix_getpeername(int socket, struct sockaddr *address, socklen_t *address_len)
{
    N_NOTICE("posix_getpeername");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(socket);
    if (!f || !f->file)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }
    Socket *s = static_cast<Socket *>(f->file);

    Endpoint* p = s->getEndpoint();
    if (s->getProtocol() == NETMAN_TYPE_TCP)
    {
        ConnectionBasedEndpoint *ce = static_cast<ConnectionBasedEndpoint *>(p);

        /// todo this may not be accurate.
        struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
        sin->sin_port = HOST_TO_BIG16(p->getRemotePort());
        sin->sin_addr.s_addr = p->getRemoteIp().getIp();
        *address_len = sizeof(struct sockaddr_in);
    }
    else
    {
        /// \todo NotConnected more correct?
        SYSCALL_ERROR(NotSupported);
        return -1;
    }

    return 0;
}
