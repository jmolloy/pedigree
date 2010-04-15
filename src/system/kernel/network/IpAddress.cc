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
        for(size_t i = 0; i < 16; i++)
        {
            if(i && ((i % 2) == 0))
            {
                if(!bZeroComp)
                    str += ":";
                
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
                }
            }
            str.append(m_Ipv6[i], 16, 2);
        }
        return String(static_cast<const char*>(str));
    }
    return String("");
}
