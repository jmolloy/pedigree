/*
 * Copyright (c) 2010 Matthew Iselin
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
#ifndef MAC_ADDRESS_H
#define MAC_ADDRESS_H

#include <processor/types.h>
#include <utilities/utility.h>

#include <utilities/StaticString.h>
#include <utilities/String.h>

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
        m_Valid = true; // it has at least a byte, which makes it partially valid
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

#endif

