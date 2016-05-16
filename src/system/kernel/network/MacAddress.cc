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

#include <network/MacAddress.h>

MacAddress::MacAddress() :
    m_Mac(), m_Valid(false)
{
    ByteSet(m_Mac, 0, 6);
}

MacAddress::MacAddress(const MacAddress &other) : MacAddress()
{
  m_Valid = other.valid();
  if (other.valid())
  {
    setMac(other.getMac());
  }
}

void MacAddress::setMac(uint8_t byte, size_t element)
{
  uint16_t word = byte;
  if (element % 2)
  {
    word <<= 8;
  }

  element /= 2;

  if(element < 3)
  {
    m_Mac[element] |= word;
    m_Valid = true; // it has at least a byte, which makes it partially valid
  }
}

void MacAddress::setMac(uint8_t element)
{
  ByteSet(m_Mac, element, 6);
  m_Valid = true;
}

void MacAddress::setMac(const uint16_t* data, bool bSwap)
{
  if(bSwap)
  {
    size_t i;
    for(i = 0; i < 3; i++)
      m_Mac[i] = BIG_TO_HOST16(data[i]);
  }
  else
    MemoryCopy(m_Mac, data, 6);

  m_Valid = true;
};

uint8_t MacAddress::getMac(size_t element) const
{
  size_t realElement = element / 2;
  if (realElement >= 3)
    return 0;

  uint16_t word = m_Mac[realElement];
  if (element % 2)
    return word >> 8;
  else
    return word & 0xFF;
};

const uint16_t* MacAddress::getMac() const
{
  return m_Mac;
};

uint8_t MacAddress::operator [] (size_t offset) const
{
  if(m_Valid)
    return getMac(offset);
  else
    return 0;
};

MacAddress& MacAddress::operator = (const MacAddress &a)
{
  if(a.valid())
    setMac(a.getMac());
  return *this;
}

MacAddress& MacAddress::operator = (const uint16_t* a)
{
  setMac(a);
  return *this;
}

String MacAddress::toString()
{
    NormalStaticString str;
    for(int i = 0; i < 6; i++)
    {
        str.append(getMac(i), 16, 2, '0');
        if(i < 5)
            str += ":";
    }
    return String(static_cast<const char*>(str));
}
