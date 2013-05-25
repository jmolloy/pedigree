/*
 * Copyright (c) 2010 Matthew Iselin, Heights College
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

#include <network/IpAddress.h>

bool IpAddress::isLinkLocal()
{
    if(m_Type == IPv4)
        return (m_Ipv4 & 0xFFFF) == 0xA9FE; // 169.254/16
    else
        return (m_Ipv6[0] == 0xFE) && (m_Ipv6[1] == 0x80);
}

String IpAddress::prefixString(size_t override)
{
    if(m_Type == IPv4)
    {
        return String("");
    }
    else
    {
        NormalStaticString str;
        str.clear();
        size_t prefix = override <= 128 ? override : m_Ipv6Prefix;

        for(size_t i = 0; i < prefix / 8; i++)
        {
            if(i && ((i % 2) == 0))
                str += ":";

            size_t pad = 1;
            if(i && m_Ipv6[i - 1]) pad = 2; // Keep internal zeroes (eg f0f).
            str.append(m_Ipv6[i], 16, pad);
        }

        return String(static_cast<const char*>(str));
    }
}

String IpAddress::toString()
{
    if(m_Type == IPv4)
    {
        NormalStaticString str;
        str.clear();
        str.append(m_Ipv4 & 0xff);
        str += ".";
        str.append((m_Ipv4 >> 8) & 0xff);
        str += ".";
        str.append((m_Ipv4 >> 16) & 0xff);
        str += ".";
        str.append((m_Ipv4 >> 24) & 0xff);
        return String(static_cast<const char*>(str));
    }
    else
    {
        NormalStaticString str;
        str.clear();
        bool bZeroComp = false;
        bool alreadyZeroComp = false; // Compression can only come once.
                                      // Naive algorithm, compresses first zeroes
                                      // but not the largest set.
        for(size_t i = 0; i < 16; i++)
        {
            if(i && ((i % 2) == 0))
            {
                if(!bZeroComp)
                    str += ":";

                if(alreadyZeroComp)
                {
                    if(m_Ipv6[i])
                        str.append(m_Ipv6[i], 16);
                    continue;
                }

                // Zero-compression
                if(!m_Ipv6[i] && !m_Ipv6[i+1])
                {
                    i++;
                    bZeroComp = true;
                    continue;
                }
                else if(bZeroComp)
                {
                    str += ":";
                    bZeroComp = false;
                    alreadyZeroComp = true;
                }
            }

            if(m_Ipv6[i] || m_Ipv6[i - 1])
            {
                size_t pad = 1;
                if(m_Ipv6[i - 1]) pad = 2; // Keep internal zeroes (eg f0f).
                str.append(m_Ipv6[i], 16, pad);
            }
        }

        // Append the prefix for this address.
        str.append("/");
        str.append(m_Ipv6Prefix, 10);

        return String(static_cast<const char*>(str));
    }
    return String("");
}

bool IpAddress::isMulticast()
{
    if(m_Type == IPv4)
    {
        // Leading bits of the IP address = 1110? The old class D is all multicast now.
        if(((m_Ipv4 & 0xFF) & 0xE0) == 0xE0)
            return true;
    }
    else if(m_Type == IPv6)
    {
        // FF00::/8 is the multicast range for IPv6.
        if(m_Ipv6[0] == 0xFF)
            return true;
    }

    return false;
}
