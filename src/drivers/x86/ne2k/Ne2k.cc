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
#include <processor/Processor.h>

Ne2k::Ne2k(Network* pDev) :
  Network(pDev), m_stationInfo(), m_pBase(0)
{
  setSpecificType(String("ne2k-card"));
  
  // grab the ports
  m_pBase = m_Addresses[0]->m_Io;
  
  // Reset the card, and clear interrupts
  m_pBase->write8(m_pBase->read8(NE_RESET), NE_RESET);
  while((m_pBase->read8(NE_ISR) & 0x80) == 0);
  m_pBase->write8(0xff, NE_ISR);
  
  // reset command
  m_pBase->write8(0x21, NE_CMD);
  
  // 16-bit transfer, monitor to avoid recv, loopback just in case
  // because we don't want to receive packets yet
  m_pBase->write8(0x09, NE_DCR);
  m_pBase->write8(0x20, NE_RCR);
  m_pBase->write8(0x02, NE_TCR);
  
  // turn off interrupts
  m_pBase->write8(0x00, NE_IMR); // ISR is already set to 0xff
  
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
  m_pBase->write8(NE_CMD, 0x61);
  for(i = 0; i < 6; i++)
  {
    m_stationInfo.mac[i] = prom[i] & 0xff;
    m_pBase->write8(prom[i] & 0xff, NE_PAR + i);
  }
  
  WARNING("NE2K: MAC is " << 
    m_stationInfo.mac[0] << ":" << 
    m_stationInfo.mac[1] << ":" << 
    m_stationInfo.mac[2] << ":" << 
    m_stationInfo.mac[3] << ":" << 
    m_stationInfo.mac[4] << ":" << 
    m_stationInfo.mac[5] << ".");
  
  // reset current page, put the card into normal mode, and set
  // packet buffer information
  m_pBase->write8(PAGE_RX + 1, NE_CURR);
  
  m_pBase->write8(0x21, NE_CMD);
  
  m_pBase->write8(PAGE_RX, NE_PSTART);
  m_pBase->write8(PAGE_RX, NE_BNDRY);
  m_pBase->write8(PAGE_STOP, NE_PSTOP);
  
  // accept broadcast and runt packets (<64 bytes)
  m_pBase->write8(0x06, NE_RCR);
  m_pBase->write8(0x00, NE_TCR);

  m_NextPacket = PAGE_RX + 1;
  
  // clear interrupts and enable them all
  m_pBase->write8(0xff, NE_ISR);
  m_pBase->write8(0x3f, NE_IMR);
  
  // start the card working properly
  m_pBase->write8(0x22, NE_CMD);
  
  // install the IRQ
  Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*> (this));
  // initialise();
  
  send(5, reinterpret_cast<uintptr_t>("Hello"));

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
  int i;
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
  for(; i < 64; i += 2) m_pBase->write8(0, NE_DATA);
  
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
    
    // packet buffer - length - 3 because extra bytes are read in
    // for status & length
    uint16_t* packBuffer = new uint16_t[length - 3];
    
    // check status, new read for the rest of the packet
    while(!(m_pBase->read8(NE_ISR) & 0x40));
    m_pBase->write8(0x40, NE_ISR);
    
    m_pBase->write8(4, NE_RSAR0);
    m_pBase->write8(m_NextPacket, NE_RSAR1);
    m_pBase->write8((length - 4) & 0xff, NE_RBCR0);
    m_pBase->write8((length - 4) >> 8, NE_RBCR1);
    m_pBase->write8(0x0a, NE_CMD);
    
    // read the packet
    int i, words = (length - 3) / 2;
    for(i = 0; i < words; i++)
      packBuffer[i] = m_pBase->read16(NE_DATA);
    if(length & 1)
      packBuffer[length - 1] = m_pBase->read8(NE_DATA); // odd packet length handler
    
    // check status once again
    while(!(m_pBase->read8(NE_ISR) & 0x40));
    m_pBase->write8(0x40, NE_ISR);
    
    // set the next packet, inform the card of the new boundary
    m_NextPacket = status >> 8;
    m_pBase->write8((m_NextPacket == PAGE_RX) ? (PAGE_STOP - 1) : (m_NextPacket - 1), NE_BNDRY);
    
    // handle the packet
    /// \note It'd be nice if this was asynchronous, at the moment this will block until the
    ///       packet is handled.
    // NetworkStack::instance().receive(reinterpret_cast<uintptr_t>(packBuffer), length);
    
    // free the used memory
    delete packBuffer;
  }

  // unmask interrupts
  m_pBase->write8(0x3f, NE_IMR);
}

bool Ne2k::setStationInfo(stationInfo info)
{
  // done manually for two reasons:
  // 1) can't do info = m_stationInfo for ipv6
  // 2) MAC isn't changeable
  m_stationInfo.ipv4 = info.ipv4;
  memcpy(m_stationInfo.ipv6, info.ipv6, 16);
}

stationInfo Ne2k::getStationInfo()
{
  return m_stationInfo;
}

bool Ne2k::irq(irq_id_t number, InterruptState &state)
{
  // grab the interrupt status
  uint8_t irqStatus = m_pBase->read8(NE_ISR);
  
  // packet received?
  if(irqStatus & 0x05)
  {
    // ack
    m_pBase->write8(0x3A, NE_IMR);
    
    WARNING("NE2K: Packet Received IRQ");
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
      m_pBase->write8(0x0A, NE_IMR);
    
      WARNING("NE2K: Packet Transmitted");
    }
  }
  
  // overflows
  if(irqStatus & 0x10)
  {
    WARNING("NE2K: Receive buffer overflow");
    m_pBase->write8(0x10, NE_IMR);
  }
  if(irqStatus & 0x20)
  {
    WARNING("NE2K: Counter overflow");
    m_pBase->write8(0x20, NE_IMR);
  }

  return false;
}
