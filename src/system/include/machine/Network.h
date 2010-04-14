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

#define MACHINE_FORWARD_DECL_ONLY
#include <machine/Machine.h>
#include <machine/Timer.h>
#undef MACHINE_FORWARD_DECL_ONLY

#include <process/Semaphore.h>
#include "../../kernel/core/BootIO.h"
extern BootIO bootIO;

#include <network/IpAddress.h>

/** The Pedigree network stack - Network timeout (for anything that blocks on a semaphore) */
class NetworkBlockTimeout : public TimerHandler
{
  public:

    NetworkBlockTimeout() : m_Nanoseconds(0), m_Seconds(0), m_Timeout(30), m_Sem(0), m_TimedOut(0) {};

    inline void setTimeout(uint32_t seconds)
    {
      m_Timeout = seconds;
    }

    /** Sets the internal semaphore, which gets released if a timeout occurs */
    inline void setSemaphore(Semaphore* sem)
    {
      m_Sem = sem;
    }

    /** Sets the internal "timed out" bool pointer, so that the timeout is not mistaken
        as a data arrival Semaphore release */
    inline void setTimedOut(bool* b)
    {
      m_TimedOut = b;
    }

    virtual void timer(uint64_t delta, InterruptState &state)
    {
      if(UNLIKELY(m_Seconds < m_Timeout))
      {
        m_Nanoseconds += delta;
        if(UNLIKELY(m_Nanoseconds >= 1000000000ULL))
        {
          ++m_Seconds;
          m_Nanoseconds -= 1000000000ULL;
        }

        if(UNLIKELY(m_Seconds >= m_Timeout))
        {
          if(m_Sem)
            m_Sem->release();
          if(m_TimedOut)
            *m_TimedOut = true;
        }
      }
    }

  private:
    uint64_t m_Nanoseconds;
    uint64_t m_Seconds;
    uint32_t m_Timeout;

    Semaphore* m_Sem;
    bool* m_TimedOut;

    NetworkBlockTimeout(const NetworkBlockTimeout& s) : m_Nanoseconds(0), m_Seconds(0), m_Timeout(30), m_Sem(0), m_TimedOut(0) {};
    NetworkBlockTimeout& operator = (const NetworkBlockTimeout& s)
    {
      ERROR("NetworkBlockTimeout copy constructor has been called.");
      return *this;
    }
};

/** A MAC address */
class MacAddress
{
  public:
    MacAddress() :
      m_Mac(), m_Valid(false)
    {};
    virtual ~MacAddress() {};

    bool valid()
    {
      return m_Valid;
    }

    void setMac(uint8_t byte, size_t element)
    {
      if(element < 6)
      {
        m_Mac[element] = byte;
        m_Valid = true; // it has, at least, a byte, which makes it partially valid
      }
    };

    /** Useful for setting a broadcast MAC */
    void setMac(uint8_t element)
    {
      memset(m_Mac, element, 6);
      m_Valid = true;
    }

    void setMac(uint8_t* data, bool bSwap = false)
    {
      if(bSwap)
      {
        uint16_t *p = reinterpret_cast<uint16_t*>(data);
        uint16_t *me = reinterpret_cast<uint16_t*>(m_Mac);
        size_t i;
        for(i = 0; i < 3; i++)
          me[i] = BIG_TO_HOST16(p[i]);
      }
      else
        memcpy(m_Mac, data, 6);
      m_Valid = true;
    };

    uint8_t getMac(size_t element)
    {
      if(element < 6)
        return m_Mac[element];
      return 0;
    };

    uint8_t* getMac()
    {
      return m_Mac;
    };

    uint8_t operator [] (size_t offset)
    {
      if(m_Valid)
        return getMac(offset);
      else
        return 0;
    };

    MacAddress& operator = (MacAddress a)
    {
      if(a.valid())
        setMac(a.getMac());
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

    String toString()
    {
        NormalStaticString str;
        for(int i = 0; i < 6; i++)
        {
            str.append(m_Mac[i], 16, 2, '0');
            if(i < 5)
                str += ":";
        }
        return String(static_cast<const char*>(str));
    }

  private:

    uint8_t   m_Mac[6];
    bool      m_Valid;
};

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
