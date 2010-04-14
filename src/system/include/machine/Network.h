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

#ifndef MACHINE_NETWORK_H
#define MACHINE_NETWORK_H

#include <processor/types.h>
#include <processor/state.h>
#include <machine/Device.h>
#include <utilities/utility.h>

#include "../../kernel/core/BootIO.h"
extern BootIO bootIO;

#include <network/IpAddress.h>
#include <network/MacAddress.h>
#include <network/NetworkBlockTimeout.h>

/** Station information - basically information about this station, per NIC */
class StationInfo
{
  public:
    StationInfo() :
      ipv4(), ipv6(IpAddress::IPv6), subnetMask(), gateway(), dnsServers(0), nDnsServers(0), mac()
    {};
    StationInfo(const StationInfo& info) :
      ipv4(info.ipv4), ipv6(info.ipv6), subnetMask(info.subnetMask), gateway(info.gateway), dnsServers(info.dnsServers), nDnsServers(info.nDnsServers), mac(info.mac)
    {};
    virtual ~StationInfo() {};

    IpAddress   ipv4;
    IpAddress   ipv6; // not compulsory

    IpAddress   subnetMask;
    IpAddress   gateway;

    IpAddress*  dnsServers;
    size_t      nDnsServers;

    MacAddress  mac; // MAC address

    StationInfo& operator = (const StationInfo& info)
    {
      WARNING("StationInfo::operator = (const StationInfo&) called");
      return *this;
    }
};

/**
 * A network device (sends/receives packets on a network)
 */
class Network : public Device
{
public:
  Network()
  {
    m_SpecificType = "Generic Network Device";
  }
  Network(Network *pDev) :
    Device(pDev)
  {
  }
  virtual ~Network()
  {
  }

  virtual Type getType()
  {
    return Device::Network;
  }

  virtual void getName(String &str)
  {
    str = "Generic Network Device";
  }

  virtual void dump(String &str)
  {
    str = "Generic Network Device";
  }

  /** Sends a given packet through the device.
   * \param nBytes The number of bytes to send.
   * \param buffer A buffer with the packet to send */
  virtual bool send(size_t nBytes, uintptr_t buffer) = 0;

  /** Sets station information (such as IP addresses)
   * \param info The information to set as the station info */
  virtual bool setStationInfo(StationInfo info)
  {
    return false; // failed by default
  }

  /** Gets station information (such as IP addresses) */
  virtual StationInfo getStationInfo()
  {
    static StationInfo info;
    return info; // not to be trusted
  }

  /** Is this device actually connected to a network? */
  virtual bool isConnected()
  {
      return true;
  }

  /** Converts an IPv4 address into an integer */
  static uint32_t convertToIpv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

  /** Calculates a checksum */
  static uint16_t calculateChecksum(uintptr_t buffer, size_t nBytes);

};

#endif
