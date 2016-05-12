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
    MacAddress();

    bool valid() const
    {
      return m_Valid;
    }

    void setMac(uint8_t byte, size_t element);

    /** Useful for setting a broadcast MAC */
    void setMac(uint8_t element);

    void setMac(const uint16_t* data, bool bSwap = false);

    uint8_t getMac(size_t element) const;

    const uint16_t* getMac() const;

    uint8_t operator [] (size_t offset) const;

    MacAddress& operator = (const MacAddress &a);

    MacAddress& operator = (const uint16_t* a);

    operator const uint16_t* () const
    {
      return getMac();
    }

    String toString();

  private:
    uint16_t  m_Mac[3];
    bool      m_Valid;
};

#endif

