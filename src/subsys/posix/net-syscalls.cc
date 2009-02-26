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
#include <network/NetManager.h>
#include <network/NetworkStack.h>

#include "file-syscalls.h"
#include "net-syscalls.h"

#define SYS_SOCK_CONSTANTS_ONLY
#include "include/sys/socket.h"
#include "include/netinet/in.h"

typedef size_t size_t;
typedef uint32_t sa_family_t;
typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct sockaddr
{
  sa_family_t sa_family;
  char sa_data[14];
};

struct in_addr
{
  in_addr_t s_addr;
};


struct sockaddr_in
{
  sa_family_t sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
};

typedef Tree<size_t,void*> FdMap;

int posix_socket(int domain, int type, int protocol)
{
  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

  size_t fd = Processor::information().getCurrentThread()->getParent()->nextFd();
  
  File file;
  bool valid = true;
  if(domain == AF_INET)
  {
    if((type == SOCK_DGRAM && protocol == 0) || (type == SOCK_DGRAM && protocol == IPPROTO_UDP))
      file = NetManager::instance().newEndpoint(NETMAN_PROTO_UDP);
    else if((type == SOCK_STREAM && protocol == 0) || (type == SOCK_STREAM && protocol == IPPROTO_TCP))
      file = NetManager::instance().newEndpoint(NETMAN_PROTO_TCP);
    else
      valid = false;
  }
  else
    valid = false;
  
  if(!valid)
    return -1;
  
  FileDescriptor *f = new FileDescriptor;
  f->file = file;
  f->offset = 0;
  fdMap.insert(fd, reinterpret_cast<void*>(f));

  return static_cast<int> (fd);
}

int fileFromSocket(int sock, File* file)
{
  /// \todo Race here - fix.
  FileDescriptor *f = reinterpret_cast<FileDescriptor*>(Processor::information().getCurrentThread()->getParent()->getFdMap().lookup(sock));

  if (!f)
  {
    SYSCALL_ERROR(BadFileDescriptor);
    return -1;
  }
  
  if(!NetManager::instance().isEndpoint(f->file))
    return -1;
  
  *file = f->file;
  
  return 0;
}

int posix_connect(int sock, struct sockaddr* address, size_t addrlen)
{
  NOTICE("posix_connect");
  
  File file;
  if(fileFromSocket(sock, &file) == -1)
    return -1;
  
  Endpoint* p = NetManager::instance().getEndpoint(file);

  Endpoint::RemoteEndpoint remoteHost;
  
  /// \todo Other protocols
  bool success = false;
  if(file.getSize() == NETMAN_PROTO_TCP)
  {
    struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
    remoteHost.remotePort = BIG_TO_HOST16(sin->sin_port);
    remoteHost.ip.setIp(sin->sin_addr.s_addr);
    
    success = p->connect(remoteHost);
  }
  else if(file.getSize() == NETMAN_PROTO_UDP)
  {
    // connect on a UDP socket sets a remote address and port for send/recv
    // to send to multiple addresses and receive from multiple clients
    // sendto and recvfrom must be used
  
    struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
    remoteHost.remotePort = BIG_TO_HOST16(sin->sin_port);
    remoteHost.ip.setIp(sin->sin_addr.s_addr);
    
    p->setRemotePort(remoteHost.remotePort);
    p->setRemoteIp(remoteHost.ip);
    success = true;
  }
  
  return success ? 0 : -1;
}

ssize_t posix_send(int sock, const void* buff, size_t bufflen, int flags)
{
  NOTICE("posix_send");
  
  File file;
  if(fileFromSocket(sock, &file) == -1)
    return -1;
  
  Endpoint* p = NetManager::instance().getEndpoint(file);
  
  bool success = false;
  if(file.getSize() == NETMAN_PROTO_TCP)
  {
    success = p->send(bufflen, reinterpret_cast<uintptr_t>(buff));
  }
  else if(file.getSize() == NETMAN_PROTO_UDP)
  {
    // special handling - need to check for a remote host
    IpAddress remoteIp = p->getRemoteIp();
    if(remoteIp.getIp() != 0)
    {
      Endpoint::RemoteEndpoint remoteHost;
      remoteHost.remotePort = p->getRemotePort();
      remoteHost.ip = remoteIp;
      p->send(bufflen, reinterpret_cast<uintptr_t>(buff), remoteHost, false, NetworkStack::instance().getDevice(0));
    }
  }
  
  return success ? bufflen : -1;
}

ssize_t posix_sendto(int sock, const void* buff, size_t bufflen, int flags, const struct sockaddr* address, size_t addrlen)
{
  NOTICE("posix_sendto");
  
  File file;
  if(fileFromSocket(sock, &file) == -1)
    return -1;
  
  Endpoint* p = NetManager::instance().getEndpoint(file);
    
  bool success = false;
  if(file.getSize() == NETMAN_PROTO_TCP)
  {
    /// \todo I need to write a sendto for TcpManager and TcpEndpoint
    ///       which probably means UDP gets a free sendto as well.
    ///       Until then, this is NOT valid according to the standards.
    ERROR("TCP sendto called, but not implemented properly!");
    success = p->send(bufflen, reinterpret_cast<uintptr_t>(buff));
  }
  else if(file.getSize() == NETMAN_PROTO_UDP)
  {
    Endpoint::RemoteEndpoint remoteHost;
    const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(address);
    remoteHost.remotePort = BIG_TO_HOST16(sin->sin_port);
    remoteHost.ip.setIp(sin->sin_addr.s_addr);
    p->send(bufflen, reinterpret_cast<uintptr_t>(buff), remoteHost, false, NetworkStack::instance().getDevice(0));
  }
  
  return success ? bufflen : -1;
}

ssize_t posix_recv(int sock, void* buff, size_t bufflen, int flags)
{
  NOTICE("posix_recv");
  
  File file;
  if(fileFromSocket(sock, &file) == -1)
    return -1;
  
  Endpoint* p = NetManager::instance().getEndpoint(file);
    
  int ret = -1;
  if(file.getSize() == NETMAN_PROTO_TCP)
  {
    /// \todo O_NONBLOCK should control the blocking nature of this call
    ret = p->recv(reinterpret_cast<uintptr_t>(buff), bufflen, false);
  }
  else if(file.getSize() == NETMAN_PROTO_UDP)
  {
    /// \todo Actually, we only should read this data if it's from the IP specified
    ///       during connect - otherwise we fail (UDP should use sendto/recvfrom)
    ///       However, to do that we need to tell recv not to remove from the queue
    ///       and instead peek at the message (in other words, we need flags)
    Endpoint::RemoteEndpoint remoteHost;
    ret = p->recv(reinterpret_cast<uintptr_t>(buff), bufflen, &remoteHost);
  }
  
  return ret;
}

ssize_t posix_recvfrom(int sock, void* buff, size_t bufflen, int flags, struct sockaddr* address, size_t* addrlen)
{
  NOTICE("posix_recvfrom");
  
  File file;
  if(fileFromSocket(sock, &file) == -1)
    return -1;
  
  Endpoint* p = NetManager::instance().getEndpoint(file);
    
  int ret = -1;
  if(file.getSize() == NETMAN_PROTO_TCP)
  {
    ret = p->recv(reinterpret_cast<uintptr_t>(buff), bufflen, false);
    
    struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
    sin->sin_port = HOST_TO_BIG16(p->getRemotePort());
    sin->sin_addr.s_addr = p->getRemoteIp().getIp();
    *addrlen = sizeof(struct sockaddr_in);
  }
  else if(file.getSize() == NETMAN_PROTO_UDP)
  {
    Endpoint::RemoteEndpoint remoteHost;
    ret = p->recv(reinterpret_cast<uintptr_t>(buff), bufflen, &remoteHost);
    
    struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
    sin->sin_port = HOST_TO_BIG16(remoteHost.remotePort);
    sin->sin_addr.s_addr = remoteHost.ip.getIp();
    *addrlen = sizeof(struct sockaddr_in);
  }
  
  return ret;
}

int posix_bind(int sock, const struct sockaddr *address, size_t addrlen)
{
  NOTICE("posix_bind");
  
  File file;
  if(fileFromSocket(sock, &file) == -1)
    return -1;
  
  Endpoint* p = NetManager::instance().getEndpoint(file);
  if(p)
  {
    int ret = -1;
    if(file.getSize() == NETMAN_PROTO_TCP || file.getSize() == NETMAN_PROTO_UDP)
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
  NOTICE("posix_listen");
  
  File file;
  if(fileFromSocket(sock, &file) == -1)
    return -1;
  
  Endpoint* p = NetManager::instance().getEndpoint(file);
  
  p->listen();
  
  return 0;
}

int posix_accept(int sock, struct sockaddr* address, size_t* addrlen)
{
  NOTICE("posix_accept");
  
  File file;
  if(fileFromSocket(sock, &file) == -1)
    return -1;
    
  File f = NetManager::instance().accept(file);
  if(f.getInode() == 0)
    return -1; // error!

  // add into the descriptor table
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

  size_t fd = Processor::information().getCurrentThread()->getParent()->nextFd();
  
  Endpoint* e = NetManager::instance().getEndpoint(f);
  
  if(address && addrlen)
  {
    if(f.getSize() == NETMAN_PROTO_TCP || f.getSize() == NETMAN_PROTO_UDP)
    {
      struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
      sin->sin_port = HOST_TO_BIG16(e->getRemotePort());
      sin->sin_addr.s_addr = e->getRemoteIp().getIp();
      
      *addrlen = sizeof(struct sockaddr_in);
    }
  }
  
  FileDescriptor *desc = new FileDescriptor;
  desc->file = f;
  desc->offset = 0;
  fdMap.insert(fd, reinterpret_cast<void*>(desc));

  return static_cast<int> (fd);
}
