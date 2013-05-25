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
#include "Ne2k.h"
#include "Ne2kConstants.h"
#include <Log.h>
#include <machine/Machine.h>
#include <machine/Network.h>
#include <network-stack/NetworkStack.h>
#include <processor/Processor.h>
#include <machine/IrqManager.h>
#include <process/Scheduler.h>

// #define NE2K_NO_THREADS

Ne2k::Ne2k(Network* pDev) :
  Network(pDev), m_pBase(0), m_NextPacket(0),
  m_PacketQueueSize(0),m_PacketQueue(), m_PacketQueueLock()
{
  setSpecificType(String("ne2k-card"));

  // grab the ports
  m_pBase = m_Addresses[0]->m_Io;

  // Reset the card, and clear interrupts
  //m_pBase->write8(m_pBase->read8(NE_RESET), NE_RESET);
  //while((m_pBase->read8(NE_ISR) & 0x80) == 0);
  //m_pBase->write8(0xff, NE_ISR);

  // reset command
  m_pBase->write8(0x21, NE_CMD);

  // 16-bit transfer, monitor to avoid recv, loopback just in case
  // because we don't want to receive packets yet
  m_pBase->write8(0x09, NE_DCR);
  m_pBase->write8(0x20, NE_RCR);
  m_pBase->write8(0x02, NE_TCR);

  // turn off interrupts
  m_pBase->write8(0xff, NE_ISR);
  m_pBase->write8(0x00, NE_IMR);

  // get the MAC from PROM
  m_pBase->write8(0x00, NE_RSAR0);
  m_pBase->write8(0x00, NE_RSAR1);

  m_pBase->write8(32,   NE_RBCR0); // 32 bytes of data
  m_pBase->write8(0,    NE_RBCR1);

  m_pBase->write8(0x0a, NE_CMD); // remote read, STOP

  uint16_t prom[16];
  int i;
  for(i = 0; i < 16; i++)
    prom[i] = m_pBase->read16(NE_DATA);

  // set the MAC address in the card itself
  m_pBase->write8(0x61, NE_CMD);
  for(i = 0; i < 6; i++)
  {
    m_StationInfo.mac.setMac(prom[i] & 0xff, i);
    m_pBase->write8(prom[i] & 0xff, NE_PAR + i);
  }

  WARNING("NE2K: MAC is " <<
    m_StationInfo.mac[0] << ":" <<
    m_StationInfo.mac[1] << ":" <<
    m_StationInfo.mac[2] << ":" <<
    m_StationInfo.mac[3] << ":" <<
    m_StationInfo.mac[4] << ":" <<
    m_StationInfo.mac[5] << ".");

  // reset current page, put the card into normal mode, and set
  // packet buffer information
  m_pBase->write8(0x61, NE_CMD);
  m_pBase->write8(PAGE_RX + 1, NE_CURR);

  m_NextPacket = PAGE_RX + 1;

  m_pBase->write8(0x21, NE_CMD);

  m_pBase->write8(PAGE_RX, NE_PSTART);
  m_pBase->write8(PAGE_RX, NE_BNDRY);
  m_pBase->write8(PAGE_STOP, NE_PSTOP);

  // accept multicast, broadcast, and runt packets (<64 bytes)
  /// \todo Proper multicast subscription via the Network card abstraction
  m_pBase->write8(0x14, NE_RCR);
  m_pBase->write8(0x00, NE_TCR);

  // Accept all multicast packets. Once we have an API for multicast subscription this will be
  // different, as we may not want to receive every single multicast packet that arrives.
  uint8_t tmp = m_pBase->read8(NE_CMD);
  m_pBase->write8(tmp | 0x40, NE_CMD);
  for(i = 0; i < MAR_SIZE; i++)
    m_pBase->write8(0xFF, NE_MAR + i);
  m_pBase->write8(tmp, NE_CMD);

  // register the packet queue handler before we install the IRQ
#ifdef THREADS
  new Thread(Processor::information().getCurrentThread()->getParent(),
             reinterpret_cast<Thread::ThreadStartFunc> (&trampoline),
             reinterpret_cast<void*> (this));
#endif

  // install the IRQ
  NOTICE("NE2K: IRQ is " << getInterruptNumber());
  Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*>(this));

  // clear interrupts and enable the ones we want
  m_pBase->write8(0xff, NE_ISR);
  m_pBase->write8(0x3D, NE_IMR); // No IRQ for "packet transmitted"

  // start the card working properly
  m_pBase->write8(0x22, NE_CMD);

  NetworkStack::instance().registerDevice(this);
}

Ne2k::~Ne2k()
{
}

bool Ne2k::send(size_t nBytes, uintptr_t buffer)
{
  if(nBytes > 0xffff)
  {
    ERROR("NE2K: Attempt to send a packet with size > 64 KB");
    return false;
  }

  // length & address for the write
  m_pBase->write8(0, NE_RSAR0);
  m_pBase->write8(PAGE_TX, NE_RSAR1);

  m_pBase->write8((nBytes > 64) ? nBytes & 0xff : 64, NE_RBCR0); // always send at least 64 bytes
  m_pBase->write8(nBytes >> 8, NE_RBCR1);

  m_pBase->write8(0x12, NE_CMD); // write, start

  uint16_t* data = reinterpret_cast<uint16_t*>(buffer);
  size_t i;
  for(i = 0; (i + 1) < nBytes; i += 2)
    m_pBase->write16(data[i/2], NE_DATA);

  // handle odd byte
  if(nBytes & 1)
  {
    if(nBytes < 64)
    {
      m_pBase->write8(data[i/2] & 0xff, NE_DATA);
      i += 2;
    }
    else
    {
      m_pBase->write8(data[i/2] & 0xff, NE_DATA);
      i++;
    }
  }

  // pad the packet to 64 bytes
  for(; i < 64; i += 2) m_pBase->write16(0, NE_DATA);

  // let it complete
  while(!(m_pBase->read8(NE_ISR) & 0x40));
  m_pBase->write8(0x40, NE_ISR);

  // execute the transmission
  m_pBase->write8((nBytes > 64) ? nBytes & 0xff : 64, NE_TBCR0);
  m_pBase->write8(nBytes >> 8, NE_TBCR1);

  m_pBase->write8(PAGE_TX, NE_TPSR);

  m_pBase->write8(0x26, NE_CMD);

  // success!
  return true;
}

void Ne2k::recv()
{
  // Grab the current buffer in the ring
  m_pBase->write8(0x61, NE_CMD);
  uint8_t current = m_pBase->read8(NE_CURR);
  m_pBase->write8(0x21, NE_CMD);

  // Read packets until the current packet
  while(m_NextPacket != current)
  {
    // Want status and length
    m_pBase->write8(0, NE_RSAR0);
    m_pBase->write8(m_NextPacket, NE_RSAR1);
    m_pBase->write8(4, NE_RBCR0);
    m_pBase->write8(0, NE_RBCR1);
    m_pBase->write8(0x0a, NE_CMD); // Read, Start

    // Grab the information we want
    uint16_t status = m_pBase->read16(NE_DATA);
    uint16_t length = m_pBase->read16(NE_DATA);

    if(!length)
    {
      ERROR("NE2K: length of packet is invalid!");
      continue;
    }

    // Remove the status and length bytes
    length -= 3;

    // packet buffer
    uint8_t* tmp = reinterpret_cast<uint8_t*>(NetworkStack::instance().getMemPool().allocate());
    uint16_t* packBuffer = reinterpret_cast<uint16_t*>(tmp);
    memset(tmp, 0, length);

    // check status, new read for the rest of the packet
    while(!(m_pBase->read8(NE_ISR) & 0x40));
    m_pBase->write8(0x40, NE_ISR);

    m_pBase->write8(4, NE_RSAR0);
    m_pBase->write8(m_NextPacket, NE_RSAR1);
    m_pBase->write8((length) & 0xff, NE_RBCR0);
    m_pBase->write8((length) >> 8, NE_RBCR1);
    m_pBase->write8(0x0a, NE_CMD);

    // read the packet
    int i, words = length / 2, oddbytes = length % 2;
    for(i = 0; i < words; ++i)
      packBuffer[i] = m_pBase->read16(NE_DATA);
    if(oddbytes)
    {
      for(i = 0; i < oddbytes; ++i)
          tmp[(length - oddbytes) + i] = m_pBase->read16(NE_DATA) & 0xFF; // odd packet length handler
    }

    // check status once again
    while(!(m_pBase->read8(NE_ISR) & 0x40)); // no interrupts at all, this wastes time...
    m_pBase->write8(0x40, NE_ISR);

    // set the next packet, inform the card of the new boundary
    m_NextPacket = status >> 8;
    m_pBase->write8((m_NextPacket == PAGE_RX) ? (PAGE_STOP - 1) : (m_NextPacket - 1), NE_BNDRY);

    // push onto the queue
    packet* p = new packet;
    p->ptr = reinterpret_cast<uintptr_t>(packBuffer);
    p->len = length;

#ifdef NE2K_NO_THREADS

    NetworkStack::instance().receive(p->len, p->ptr, this, 0);

    NetworkStack::instance().getMemPool().free(p->ptr);
    delete p;

#else

    {
        LockGuard<Spinlock> guard(m_PacketQueueLock);
        m_PacketQueue.pushBack(p);
        m_PacketQueueSize.release();
    }

#endif
  }
}

int Ne2k::trampoline(void *p)
{
  Ne2k *pNe = reinterpret_cast<Ne2k*> (p);
  pNe->receiveThread();
  return 0;
}

void Ne2k::receiveThread()
{
  while(true)
  {
    // handle the incoming packet
    m_PacketQueueSize.acquire();

    // grab from the front
    packet* p = 0;
    {
        LockGuard<Spinlock> guard(m_PacketQueueLock);
        p = m_PacketQueue.popFront();
    }

    if(!p)
        continue;
    else if(!p->ptr || !p->len)
        continue;

    // pass to the network stack
    NetworkStack::instance().receive(p->len, p->ptr, this, 0);

    // destroy the buffer now that it's handled
    NetworkStack::instance().getMemPool().free(p->ptr);
    delete p;
  }
}

bool Ne2k::setStationInfo(StationInfo info)
{
  // free the old DNS servers list, if there is one
  if(m_StationInfo.dnsServers)
    delete [] m_StationInfo.dnsServers;

  // MAC isn't changeable, so set it all manually
  m_StationInfo.ipv4 = info.ipv4;
  NOTICE("NE2K: Setting ipv4, " << info.ipv4.toString() << ", " << m_StationInfo.ipv4.toString() << "...");

  m_StationInfo.ipv6 = info.ipv6;
  m_StationInfo.nIpv6Addresses = info.nIpv6Addresses;
  NOTICE("NE2K: Copied " << info.nIpv6Addresses << " IPv6 addresses.");

  m_StationInfo.subnetMask = info.subnetMask;
  NOTICE("NE2K: Setting subnet mask, " << info.subnetMask.toString() << ", " << m_StationInfo.subnetMask.toString() << "...");
  m_StationInfo.gateway = info.gateway;
  NOTICE("NE2K: Setting gateway, " << info.gateway.toString() << ", " << m_StationInfo.gateway.toString() << "...");

  // Callers do not free their dnsServers memory
  m_StationInfo.dnsServers = info.dnsServers;
  m_StationInfo.nDnsServers = info.nDnsServers;
  NOTICE("NE2K: Setting DNS servers [" << Dec << m_StationInfo.nDnsServers << Hex << " servers being set]...");

  return true;
}

StationInfo Ne2k::getStationInfo()
{
  return m_StationInfo;
}

bool Ne2k::irq(irq_id_t number, InterruptState &state)
{
  // Grab the interrupt status
  uint8_t irqStatus = m_pBase->read8(NE_ISR);

  // Handle packet received - recv will handle all interim packets as well
  // as the one which triggered the IRQ
  if(irqStatus & 0x05)
  {
    m_pBase->write8(0x3D, NE_IMR); // RECV
    recv();
    m_pBase->write8(0x3D, NE_IMR); // Enable recv interrupts again
  }

  // Handle packet transmitted
  if(irqStatus & 0x0A)
  {
    // Failure?
    if(irqStatus & 0x8)
      WARNING("NE2K: Packet transmit failed!");
  }

  // Overflows
  /// \todo Handle properly
  if(irqStatus & 0x10)
  {
    WARNING("NE2K: Receive buffer overflow");
  }
  if(irqStatus & 0x20)
  {
    WARNING("NE2K: Counter overflow");
  }

  // Ack all status items
  m_pBase->write8(irqStatus, NE_ISR);

  return true;
}

bool Ne2k::isConnected()
{
    // The NE2K chip doesn't support detecting the link state, so we have to
    // just assume the link is active.
    return true;
}
