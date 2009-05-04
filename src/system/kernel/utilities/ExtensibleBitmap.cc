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

#include <utilities/ExtensibleBitmap.h>
#include <utilities/utility.h>

ExtensibleBitmap::ExtensibleBitmap() :
    m_StaticMap(0), m_pDynamicMap(0), m_DynamicMapSize(0), m_MaxBit(0)
{
}

ExtensibleBitmap::ExtensibleBitmap(const ExtensibleBitmap &other) :
    m_StaticMap(other.m_StaticMap), m_pDynamicMap(0),
    m_DynamicMapSize(other.m_DynamicMapSize), m_MaxBit(other.m_MaxBit)
{
    m_pDynamicMap = new uint8_t[m_DynamicMapSize];
    memcpy(m_pDynamicMap, other.m_pDynamicMap, m_DynamicMapSize);
}

ExtensibleBitmap &ExtensibleBitmap::operator = (const ExtensibleBitmap &other)
{
    m_StaticMap = other.m_StaticMap;
    
    if (m_DynamicMapSize < other.m_DynamicMapSize)
    {
        uint8_t *pMap = new uint8_t[other.m_DynamicMapSize];
        memset(pMap, 0, other.m_DynamicMapSize);
        if (m_DynamicMapSize)
            delete [] m_pDynamicMap;
        m_pDynamicMap = pMap;
        m_DynamicMapSize = other.m_DynamicMapSize;
    }

    if (other.m_DynamicMapSize)
        memcpy(m_pDynamicMap, other.m_pDynamicMap, other.m_DynamicMapSize);
    m_MaxBit = other.m_MaxBit;

    return *this;
}

ExtensibleBitmap::~ExtensibleBitmap()
{
    if (m_pDynamicMap)
        delete [] m_pDynamicMap;
}

void ExtensibleBitmap::set(size_t n)
{
    if (n < sizeof(uintptr_t)*8)
        m_StaticMap |= (1 << n);

    n -= sizeof(uintptr_t)*8;

    // Check we have enough space to handle the bit.
    if (n/8 >= m_DynamicMapSize)
    {
        // Add another 8 bytes as a performance hint.
        size_t sz = n/8 + 8;
        uint8_t *pMap = new uint8_t[sz];
        memset(pMap, 0, sz);
        if (m_DynamicMapSize)
        {
            memcpy(pMap, m_pDynamicMap, m_DynamicMapSize);
            delete [] m_pDynamicMap;
        }
        m_pDynamicMap = pMap;
        m_DynamicMapSize = sz;
    }

    m_pDynamicMap[n/8] |= (1 << (n%8));
    if (n > m_MaxBit)
        m_MaxBit = n;
}

void ExtensibleBitmap::clear(size_t n)
{
    if (n < sizeof(uintptr_t)*8)
        m_StaticMap &= ~(1 << n);

    n -= sizeof(uintptr_t)*8;

    // If its outside the range of possible set bits, it must be clear already.
    if (n > m_MaxBit)
        return;
    m_pDynamicMap[n/8] &= ~(1 << (n%8));
}

bool ExtensibleBitmap::test(size_t n)
{
    if (n < sizeof(uintptr_t)*8)
        return (m_StaticMap & (1 << n));

    n -= sizeof(uintptr_t)*8;

    // If its outside the range of possible set bits, it must be clear.
    if (n > m_MaxBit)
        return false;
    return (m_pDynamicMap[n/8] & (1 << (n%8)));
}
