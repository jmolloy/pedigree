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
#include <network/NetworkStack.h>
#include <processor/Processor.h>

Ne2k::Ne2k(Network* pDev) :
  Network(pDev), m_pBase(0), m_StationInfo(), m_NextPacket(0), m_PacketQueueSize(0), m_PacketQueue()
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
  m_pBase->write8(PAGE_RX + 1, NE_CURR);

  m_NextPacket = PAGE_RX + 1;
  
  m_pBase->write8(0x21, NE_CMD);
  
  m_pBase->write8(PAGE_RX, NE_PSTART);
  m_pBase->write8(PAGE_RX, NE_BNDRY);
  m_pBase->write8(PAGE_STOP, NE_PSTOP);
  
  // accept broadcast and runt packets (<64 bytes)
  m_pBase->write8(0x06, NE_RCR);
  m_pBase->write8(0x00, NE_TCR);

  // register the packet queue handler before we install the IRQ
#ifdef THREADS
  new Thread(Processor::information().getCurrentThread()->getParent(),
             reinterpret_cast<Thread::ThreadStartFunc> (&trampoline),
             reinterpret_cast<void*> (this));
#endif
  
  // install the IRQ
  Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*> (this));
  
  // clear interrupts and enable them all
  m_pBase->write8(0xff, NE_ISR);
  m_pBase->write8(0x3f, NE_IMR);
  
  // start the card working properly
  m_pBase->write8(0x22, NE_CMD);
  
  NetworkStack::instance().registerDevice(this);
}

Ne2k::~Ne2k()
{
}

bool Ne2k::send(uint32_t nBytes, uintptr_t buffer)
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
  // check for error
  uint8_t isr = m_pBase->read8(NE_ISR);
  if(isr & 0x4)
  {
    ERROR("NE2K: Receive failed [status=" << isr << "]!");
    return;
  }
  
  // acknowledge the interrupt, fetch the current counter
  m_pBase->write8(0x01, NE_ISR);

  m_pBase->write8(0x61, NE_CMD);
  uint8_t current = m_pBase->read8(NE_CURR);
  m_pBase->write8(0x21, NE_CMD);
  
  // read packets until the current packet
  while(m_NextPacket != current)
  {      
    // need status and length
    m_pBase->write8(0, NE_RSAR0);
    m_pBase->write8(m_NextPacket, NE_RSAR1);
    m_pBase->write8(4, NE_RBCR0);
    m_pBase->write8(0, NE_RBCR1);
    m_pBase->write8(0x0a, NE_CMD); // read, start
    
    uint16_t status = m_pBase->read16(NE_DATA);
    uint16_t length = m_pBase->read16(NE_DATA);
        
    if(!length)
    {
      ERROR("NE2K: length of packet is invalid!");        
      //break;
      return;
    }
    
    // remove the status and length bytes
    length -= 3;
    
    // packet buffer
    uint8_t* tmp = new uint8_t[length];
    uint16_t* packBuffer = reinterpret_cast<uint16_t*>(tmp);
    memset(packBuffer, 0, length);
    
    // check status, new read for the rest of the packet
    while(!(m_pBase->read8(NE_ISR) & 0x40));
    m_pBase->write8(0x40, NE_ISR);
    
    m_pBase->write8(4, NE_RSAR0);
    m_pBase->write8(m_NextPacket, NE_RSAR1);
    m_pBase->write8((length) & 0xff, NE_RBCR0);
    m_pBase->write8((length) >> 8, NE_RBCR1);
    m_pBase->write8(0x0a, NE_CMD);
    
    // read the packet
    int i, words = (length) / 2;
    for(i = 0; i < words; ++i)
      packBuffer[i] = m_pBase->read16(NE_DATA);
    if(length & 1)
      packBuffer[length - 1] = m_pBase->read16(NE_DATA) & 0xFF; // odd packet length handler
    
    // check status once again
    while(!(m_pBase->read8(NE_ISR) & 0x40)); // no interrupts at all, this wastes time...
    m_pBase->write8(0x40, NE_ISR);
    
    // set the next packet, inform the card of the new boundary
    m_NextPacket = status >> 8;
    m_pBase->write8((m_NextPacket == PAGE_RX) ? (PAGE_STOP - 1) : (m_NextPacket - 1), NE_BNDRY);
    
    // push onto the queue
    packet* p = new packet;
    p->ptr = reinterpret_cast<uintptr_t>(packBuffer);
    p->len = (i * 2) + ((length & 1) ? 1 : 0); //length;
    m_PacketQueue.pushBack(p);
    m_PacketQueueSize.release();
  }

  // unmask interrupts
  m_pBase->write8(0x3f, NE_IMR);
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
    packet* p = m_PacketQueue.popFront();
    uint8_t* packBuffer = reinterpret_cast<uint8_t*>(p->ptr);
    
    // pass to the network stack
    NetworkStack::instance().receive(p->len, p->ptr, this, 0);
    
    // destroy the buffer now that it's handled
    delete packBuffer;
    delete p;
  }
}

bool Ne2k::setStationInfo(StationInfo info)
{
  // MAC isn't changeable, so set it all manually
  m_StationInfo.ipv4 = info.ipv4;
  NOTICE("Setting ipv4, " << info.ipv4.getIp() << ", " << m_StationInfo.ipv4.getIp() << "...");
  m_StationInfo.ipv6 = info.ipv6;
  
  m_StationInfo.subnetMask = info.subnetMask;
  NOTICE("Setting subnet mask, " << info.subnetMask.getIp() << ", " << m_StationInfo.subnetMask.getIp() << "...");
  m_StationInfo.gateway = info.gateway;
  NOTICE("Setting gateway, " << info.gateway.getIp() << ", " << m_StationInfo.gateway.getIp() << "...");
  
  return true;
}

StationInfo Ne2k::getStationInfo()
{
  return m_StationInfo;
}

bool Ne2k::irq(irq_id_t number, InterruptState &state)
{
  // grab the interrupt status
  uint8_t irqStatus = m_pBase->read8(NE_ISR);
  
  // packet received?
  if(irqStatus & 0x05)
  {
    // unmask all except the receive irq, because we're handling it
    m_pBase->write8(0x3A, NE_IMR);
    
    //NOTICE("NE2K: Packet Received");
    recv();
  }
  
  // packet transmitted?
  if(irqStatus & 0x0A)
  {
    // check for failure
    if(m_pBase->read8(NE_ISR) & 0x8)
      WARNING("NE2K: Packet transmit failed!");
    else
    {
      // ack
      m_pBase->write8(0x0A, NE_ISR);
    
      //NOTICE("NE2K: Packet Transmitted");
    }
  }
  
  // overflows
  if(irqStatus & 0x10)
  {
    WARNING("NE2K: Receive buffer overflow");
    m_pBase->write8(0x10, NE_ISR);
  }
  if(irqStatus & 0x20)
  {
    WARNING("NE2K: Counter overflow");
    m_pBase->write8(0x20, NE_ISR);
  }

  return true;
}
