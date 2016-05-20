/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef IP_ADDRESS_H
#define IP_ADDRESS_H

#include <processor/types.h>
#include <utilities/utility.h>

#include <utilities/StaticString.h>
#include <utilities/String.h>

/** An IPv4/IPv6 address */
class IpAddress
{
    public:

    /** The type of an IP address */
    enum IpType
    {
        IPv4 = 0x0800,
        IPv6 = 0x86DD
    };

    /** Constructors */
    IpAddress() :
        m_Type(IPv4), m_Ipv4(0), m_Ipv6(), m_bSet(false), m_Ipv6Prefix(128)
    {}
    IpAddress(IpType type) :
        m_Type(type), m_Ipv4(0), m_Ipv6(), m_bSet(false), m_Ipv6Prefix(128)
    {}
    IpAddress(uint32_t ipv4) :
        m_Type(IPv4), m_Ipv4(ipv4), m_Ipv6(), m_bSet(true), m_Ipv6Prefix(128)
    {}
    IpAddress(uint8_t *ipv6) :
        m_Type(IPv6), m_Ipv4(0), m_Ipv6(), m_bSet(true), m_Ipv6Prefix(128)
    {
        MemoryCopy(m_Ipv6, ipv6, 16);
    }
    IpAddress(const IpAddress &other) : IpAddress()
    {
        if(other.getType() == IPv6)
        {
            other.getIp(m_Ipv6);
            m_Ipv4 = 0;
            m_Type = IPv6;
            m_Ipv6Prefix = other.getIpv6Prefix();
            m_bSet = true;
        }
        else
        {
            setIp(other.getIp());
        }
    }

    /** IP setters - only one type is valid at any one point in time */

    void setIp(uint32_t ipv4)
    {
        m_Ipv4 = ipv4;
        ByteSet(m_Ipv6, 0, 16);
        m_bSet = true;
        m_Type = IPv4;
    }

    void setIp(uint8_t *ipv6)
    {
        m_Ipv4 = 0;
        MemoryCopy(m_Ipv6, ipv6, 16);
        m_bSet = true;
        m_Type = IPv6;
    }

    /** IP getters */

    inline uint32_t getIp() const
    {
        return m_Ipv4;
    }

    inline void getIp(uint8_t *ipv6) const
    {
        if(ipv6)
            MemoryCopy(ipv6, m_Ipv6, 16);
    }

    /** Type getter */

    IpType getType() const
    {
        return m_Type;
    }

    /** Operators */

    IpAddress operator + (const IpAddress &a) const
    {
        if(a.getType() != m_Type || m_Type == IPv6 || a.getType() == IPv6)
            return IpAddress();
        else
        {
            IpAddress ret(getIp() + a.getIp());
            return ret;
        }
    }

    IpAddress operator & (const IpAddress &a) const
    {
        if(a.getType() != m_Type || m_Type == IPv6 || a.getType() == IPv6)
            return IpAddress();
        else
        {
            IpAddress ret(getIp() & a.getIp());
            return ret;
        }
    }

    IpAddress& operator = (const IpAddress &a)
    {
        if(a.getType() == IPv6)
        {
            a.getIp(m_Ipv6);
            m_Ipv4 = 0;
            m_Type = IPv6;
            m_Ipv6Prefix = a.getIpv6Prefix();
            m_bSet = true;
        }
        else
            setIp(a.getIp());
        return *this;
    }

    bool operator == (const IpAddress &a) const
    {
        if(a.getType() == IPv4)
            return (a.getIp() == getIp());
        else
        {
            return !MemoryCompare(m_Ipv6, a.m_Ipv6, 16);
        }
    }

    bool operator != (const IpAddress &a) const
    {
        if(a.getType() == IPv4)
            return (a.getIp() != getIp());
        else
        {
            return !MemoryCompare(m_Ipv6, a.m_Ipv6, 16);
        }
    }

    bool operator == (uint32_t a) const
    {
        if(getType() == IPv4)
            return (a == getIp());
        else
            return false;
    }

    bool operator != (uint32_t a) const
    {
        if(getType() != IPv4)
            return (a != getIp());
        else
            return false;
    }

    String toString() const;

    /// Prefix string. Not zero-compressed. For routing.
    String prefixString(size_t override = 256) const;

    /// Whether the IP address is considered "link-local" or not.
    bool isLinkLocal() const;

    /// Whether the IP address is a valid multicast address.
    bool isMulticast() const;

    /// Whether the IP address is a valid unicast address.
    inline bool isUnicast() const { return !isMulticast(); }

    /// Obtains the IPv6 prefix for this address.
    inline size_t getIpv6Prefix() const
    {
        return m_Ipv6Prefix;
    }

    /// Sets the IPv6 prefix for this address.
    inline void setIpv6Prefix(size_t prefix)
    {
        m_Ipv6Prefix = prefix;
    }

    private:

        IpType      m_Type;

        uint32_t    m_Ipv4; // the IPv4 address
        uint8_t     m_Ipv6[16]; // the IPv6 address

        bool        m_bSet; // has the IP been set yet?

        size_t      m_Ipv6Prefix; // IPv6 prefix for this IP.
};

#endif

