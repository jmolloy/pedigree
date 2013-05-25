/*
 * Copyright (c) 2010 James Molloy, Jörg Pfähler, Matthew Iselin,
 *                    Heights College
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

#include <machine/Network.h>
#include <processor/IoPort.h>
#include <processor/MemoryMappedIo.h>
#include <processor/PhysicalMemoryManager.h>
#include <Log.h>

uint32_t Network::convertToIpv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  return a | (b << 8) | (c << 16) | (d << 24);
}

IpAddress Network::convertToIpv6(
        uint8_t a, uint8_t b,
        uint8_t c, uint8_t d,
        uint8_t e, uint8_t f,
        uint8_t g, uint8_t h,
        uint8_t i, uint8_t j,
        uint8_t k, uint8_t l,
        uint8_t m, uint8_t n,
        uint8_t o, uint8_t p
)
{
    uint8_t temp[16] = {
                            a, b,
                            c, d,
                            e, f,
                            g, h,
                            i, j,
                            k, l,
                            m, n,
                            o, p
    };
    IpAddress ipv6(temp);
    return temp;
}

uint16_t Network::calculateChecksum(uintptr_t buffer, size_t nBytes)
{
  uint32_t sum = 0;
  uint16_t* data = reinterpret_cast<uint16_t*>(buffer);

  while(nBytes > 1)
  {
    sum += *data++;
    nBytes -= sizeof( uint16_t );
  }

  // odd bytes
  if(nBytes > 0)
    sum += (*data++) & 0xFF;

  // fold to 16 bits
  while( sum >> 16 )
    sum = ( sum & 0xFFFF ) + ( sum >> 16 );

  uint16_t ret = static_cast<uint16_t>(~sum);
  return ret;
}
