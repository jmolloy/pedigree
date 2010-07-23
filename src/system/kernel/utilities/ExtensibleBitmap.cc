/*
 * Copyright (c) 2010 James Molloy, Eduard Burtescu
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
#include <Log.h>

ExtensibleBitmap::ExtensibleBitmap() :
    m_StaticMap(0), m_pDynamicMap(0), m_DynamicMapSize(0), m_nMaxBit(0),
    m_nFirstSetBit(0), m_nFirstClearBit(0), m_nLastSetBit(0), m_nLastClearBit(0)
{
}

ExtensibleBitmap::ExtensibleBitmap(const ExtensibleBitmap &other) :
    m_StaticMap(other.m_StaticMap), m_pDynamicMap(0),
    m_DynamicMapSize(other.m_DynamicMapSize), m_nMaxBit(other.m_nMaxBit),
    m_nFirstSetBit(other.m_nFirstSetBit), m_nFirstClearBit(other.m_nFirstClearBit),
    m_nLastSetBit(other.m_nLastSetBit), m_nLastClearBit(other.m_nLastClearBit)
{
    m_pDynamicMap = new uint8_t[m_DynamicMapSize];
    memcpy(m_pDynamicMap, other.m_pDynamicMap, m_DynamicMapSize);
}

ExtensibleBitmap &ExtensibleBitmap::operator = (const ExtensibleBitmap &other)
{
    m_StaticMap = other.m_StaticMap;

    if(m_DynamicMapSize < other.m_DynamicMapSize)
    {
        uint8_t *pMap = new uint8_t[other.m_DynamicMapSize];
        memset(pMap, 0, other.m_DynamicMapSize);
        if(m_DynamicMapSize)
            delete [] m_pDynamicMap;
        m_pDynamicMap = pMap;
        m_DynamicMapSize = other.m_DynamicMapSize;
    }

    if(other.m_DynamicMapSize)
        memcpy(m_pDynamicMap, other.m_pDynamicMap, other.m_DynamicMapSize);
    m_nMaxBit = other.m_nMaxBit;
    m_nFirstSetBit = other.m_nFirstSetBit;
    m_nFirstClearBit = other.m_nFirstClearBit;
    m_nLastSetBit = other.m_nLastSetBit;
    m_nLastClearBit = other.m_nLastClearBit;

    return *this;
}

ExtensibleBitmap::~ExtensibleBitmap()
{
    if(m_pDynamicMap)
        delete [] m_pDynamicMap;
}

void ExtensibleBitmap::set(size_t n)
{
    // Check if the bit we'll set becomes the first set bit
    if(n < m_nFirstSetBit)
        m_nFirstSetBit = n;
    // Check if the bit we'll set becomes the last set bit
    if(n > m_nLastSetBit)
        m_nLastSetBit = n;
    // Check if the bit we'll set replaces the first clear bit
    if(n == m_nFirstClearBit)
    {
        do
        {
            m_nFirstClearBit++;
        }
        while(test(m_nFirstClearBit));
    }

    /*if(n == m_nLastClearBit)
    {
        for(size_t i = n;i;i--)
        {
            if(!test(i))
            {
                m_nLastClearBit = i;
                break;
            }
        }
    }*/

    if(n < sizeof(uintptr_t)*8)
    {
        m_StaticMap |= (1 << n);
        return;
    }

    n -= sizeof(uintptr_t)*8;

    // Check we have enough space to handle the bit.
    if(n/8 >= m_DynamicMapSize)
    {
        // Add another 8 bytes as a performance hint.
        size_t sz = n/8 + 8;
        uint8_t *pMap = new uint8_t[sz];
        memset(pMap, 0, sz);
        if(m_DynamicMapSize)
        {
            memcpy(pMap, m_pDynamicMap, m_DynamicMapSize);
            delete [] m_pDynamicMap;
        }
        m_pDynamicMap = pMap;
        m_DynamicMapSize = sz;
    }

    m_pDynamicMap[n/8] |= (1 << (n%8));
    if(n > m_nMaxBit)
        m_nMaxBit = n;
}

void ExtensibleBitmap::clear(size_t n)
{
    // Check if the bit we'll clear becomes the first clear bit
    if(n < m_nFirstClearBit)
        m_nFirstClearBit = n;
    // Check if the bit we'll clear replaces the first set bit
    if(n == m_nFirstSetBit)
    {
        for(size_t i = n;i<=sizeof(uintptr_t)*8+m_nMaxBit;i++)
        {
            if(test(i))
            {
                m_nFirstSetBit = i;
                break;
            }
        }
    }
    // Check if the bit we'll clear replaces the last set bit
    if(n == m_nLastSetBit)
    {
        // In case we won't find any set bit, this assures any bit set later will become the last set bit
        m_nLastSetBit = 0;
        for(size_t i = n;i;i--)
        {
            if(test(i))
            {
                m_nLastSetBit = i;
                break;
            }
        }
    }

    if(n < sizeof(uintptr_t)*8)
    {
        m_StaticMap &= ~(1 << n);
        return;
    }

    n -= sizeof(uintptr_t)*8;

    // If its outside the range of possible set bits, it must be clear already.
    if(n > m_nMaxBit)
        return;
    m_pDynamicMap[n/8] &= ~(1 << (n%8));
}

bool ExtensibleBitmap::test(size_t n)
{
    if(n < sizeof(uintptr_t)*8)
        return (m_StaticMap & (1 << n));

    n -= sizeof(uintptr_t)*8;

    // If its outside the range of possible set bits, it must be clear.
    if(n > m_nMaxBit || !m_pDynamicMap)
        return false;
    return (m_pDynamicMap[n/8] & (1 << (n%8)));
}
