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

#include <machine/Device.h>
#include <utilities/utility.h>

/** A MAC address */
class MacAddress
{
  public:
    MacAddress() :
      mac()
    {};
    virtual ~MacAddress() {};
    
    void setMac(uint8_t byte, size_t element)
    {
      if(element < 6)
        mac[element] = byte;
    };
    
    void setMac(uint8_t* data)
    {
      memcpy(mac, data, 6);
    };
    
    uint8_t getMac(size_t element)
    {
      if(element < 6)
        return mac[element];
      return 0;
    };
    
    uint8_t* getMac()
    {
      return mac;
    };
    
    uint8_t operator [] (size_t offset)
    {
      return getMac(offset);
    };
    
    MacAddress& operator = (MacAddress a)
    {
      memcpy(mac, a.getMac(), 6);
      return *this;
    }
    
    MacAddress& operator = (uint8_t* a)
    {
      setMac(a);
      return *this;
    }
    
    operator uint8_t* ()
    {
      return getMac();
    }
    
  private:
  
    uint8_t   mac[6];
};

/** An IP address *
class IpAddress
{
  public:
  
  private:
};
*/

/** Station information */
struct stationInfo
{
  stationInfo() :
    ipv4(0), ipv6(), mac()
  {};
  
  uint32_t    ipv4;     // ipv4 address
  uint8_t     ipv6[16]; // ipv6 address (not compulsory)
  
  uint32_t    subnetMask;
  uint32_t    gateway;
  
  MacAddress  mac;      // MAC address
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
  virtual bool send(uint32_t nBytes, uintptr_t buffer)
  {
    return false; // failed by default
  }
  
  /** Sets station information (such as IP addresses)
   * \param info The information to set as the station info */
  virtual bool setStationInfo(stationInfo info)
  {
    return false; // failed by default
  }

  /** Gets station information (such as IP addresses) */
  virtual stationInfo getStationInfo()
  {
    static stationInfo info;
    return info; // not to be trusted
  }
  
  /** Converts an IPv4 address into an integer */
  static uint32_t convertToIpv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
  
  /** Calculates a checksum */
  static uint16_t calculateChecksum(uintptr_t buffer, size_t nBytes);

};

#endif
