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

#include "NetworkStack.h"
#include "Ethernet.h"
#include "Ipv6.h"
#include <Module.h>
#include <Log.h>
#include <processor/Processor.h>

#include "Dns.h"

NetworkStack NetworkStack::stack;

NetworkStack::NetworkStack() :
  RequestQueue(), m_pLoopback(0), m_Children(), m_MemPool("network-pool")
{
  initialise();

#if defined(X86_COMMON)
  // Lots of RAM to burn! Try 16 MB, then 8 MB, then 4 MB, then give up
  if(!m_MemPool.initialise(4096, 1600))
      if(!m_MemPool.initialise(2048, 1600))
        if(!m_MemPool.initialise(1024, 1600))
            ERROR("Couldn't get a valid buffer pool for networking use");
#elif defined(ARM_COMMON)
  // Probably very little RAM to burn - 4 MB then 2 MB, then 512 KB
  NOTICE_NOLOCK("allocating memory pool");
  if(!m_MemPool.initialise(1024, 1600))
      if(!m_MemPool.initialise(512, 1600))
        if(!m_MemPool.initialise(128, 1600))
            ERROR("Couldn't get a valid buffer pool for networking use");
#else
#warning Unhandled architecture for the NetworkStack buffer pool
#endif
}

NetworkStack::~NetworkStack()
{
  destroy();
}

uint64_t NetworkStack::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                                uint64_t p6, uint64_t p7, uint64_t p8)
{
    ERROR("Old style network stack request, ignoring");

    /*

    // Make sure we have interrupts enabled, so we're interruptible.
    Processor::setInterrupts(true);

    uintptr_t packet = static_cast<uintptr_t>(p1);
    size_t packetSize = static_cast<size_t>(p2);
    Network *pCard = reinterpret_cast<Network*>(p3);
    uint32_t offset = static_cast<uint32_t>(p4);

    if(!packet || !packetSize)
        return 0;

    // Pass onto the ethernet layer
    /// \todo We should accept a parameter here that specifies the type of packet
    ///       so we can pass it on to the correct handler, rather than assuming
    ///       Ethernet.
    Ethernet::instance().receive(packetSize, packet, pCard, offset);

    m_MemPool.free(packet);

    */

    return 0;
}

int NetworkStack::packetThread(void *p)
{
    Packet *pack = reinterpret_cast<Packet*>(p);
    if(!pack)
        return 0;

    uintptr_t packet = pack->buffer;
    size_t packetSize = pack->packetLength;
    Network *pCard = pack->pCard;
    uint32_t offset = pack->offset;

    if(!packet || !packetSize)
    {
        delete pack;
        return 0;
    }

    // Pass onto the ethernet layer
    /// \todo We should accept a parameter here that specifies the type of packet
    ///       so we can pass it on to the correct handler, rather than assuming
    ///       Ethernet.
    Ethernet::instance().receive(packetSize, packet, pCard, offset);

    NetworkStack::instance().getMemPool().free(packet);

    delete pack;
    return 0;
}

void NetworkStack::receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{
  if(!packet || !nBytes)
      return;

  pCard->gotPacket();

  // Some cards might be giving us a DMA address or something, so we copy
  // before passing on to the worker thread...
  uint8_t *safePacket = reinterpret_cast<uint8_t*>(m_MemPool.allocateNow());
  if(!safePacket)
  {
    ERROR("Network Stack: Out of memory pool space, dropping incoming packet");
    pCard->droppedPacket();
    return;
  }
  memcpy(safePacket, reinterpret_cast<void*>(packet), nBytes);
  Packet *p = new Packet;
  p->buffer = reinterpret_cast<uintptr_t>(safePacket);
  p->packetLength = nBytes;
  p->pCard = pCard;
  p->offset = offset;

  new Thread(Processor::information().getCurrentThread()->getParent(), packetThread, p);

  /*
  addAsyncRequest(0, reinterpret_cast<uint64_t>(safePacket),
                     static_cast<uint64_t>(nBytes),
                     reinterpret_cast<uint64_t>(pCard),
                     static_cast<uint64_t>(offset));
  */
}

void NetworkStack::registerDevice(Network *pDevice)
{
  m_Children.pushBack(pDevice);
}

Network *NetworkStack::getDevice(size_t n)
{
  return m_Children[n];
}

size_t NetworkStack::getNumDevices()
{
  return m_Children.count();
}

void NetworkStack::deRegisterDevice(Network *pDevice)
{
  int i = 0;
  for(Vector<Network*>::Iterator it = m_Children.begin();
      it != m_Children.end();
      it++, i++)
  if (*it == pDevice)
  {
    m_Children.erase(it);
    break;
  }
}

extern Ipv6Service *g_pIpv6Service;
extern ServiceFeatures *g_pIpv6Features;

static void entry()
{
    // Initialise the DNS implementation
    Dns::instance().initialise();

    // Install the IPv6 Service
    g_pIpv6Service = new Ipv6Service;
    g_pIpv6Features = new ServiceFeatures;
    g_pIpv6Features->add(ServiceFeatures::touch);
    ServiceManager::instance().addService(String("ipv6"), g_pIpv6Service, g_pIpv6Features);
}

static void exit()
{
}

MODULE_INFO("network-stack", &entry, &exit, "vfs");
