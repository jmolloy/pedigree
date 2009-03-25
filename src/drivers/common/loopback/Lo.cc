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

#include "Lo.h"
#include <Log.h>
#include <machine/Machine.h>
#include <machine/Network.h>
#include <network/NetworkStack.h>
#include <processor/Processor.h>

Loopback Loopback::m_Instance;

Loopback::Loopback() :
  Network(), m_StationInfo()
{
  setSpecificType(String("Loopback-card"));
  NetworkStack::instance().registerDevice(this);
  
  m_StationInfo.ipv4.setIp(Network::convertToIpv4(127, 0, 0, 1));
  m_StationInfo.mac.setMac(static_cast<uint8_t>(0));
}

Loopback::Loopback(Network* pDev) :
  Network(pDev), m_StationInfo()
{
  setSpecificType(String("Loopback-card"));
  NetworkStack::instance().registerDevice(this);
  
  m_StationInfo.ipv4.setIp(Network::convertToIpv4(127, 0, 0, 1));
  m_StationInfo.mac.setMac(static_cast<uint8_t>(0));
}

Loopback::~Loopback()
{
}

bool Loopback::send(uint32_t nBytes, uintptr_t buffer)
{
  if(nBytes > 0xffff)
  {
    ERROR("Loopback: Attempt to send a packet with size > 64 KB");
    return false;
  }
  NetworkStack::instance().receive(nBytes, buffer, this, 0);
  return true;
}

bool Loopback::setStationInfo(StationInfo info)
{
  // nothing here is modifiable
  return true;
}

StationInfo Loopback::getStationInfo()
{
  return m_StationInfo;
}
