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
#ifndef EXTENSIBLE_BITMAP_H
#define EXTENSIBLE_BITMAP_H

#include <processor/types.h>

/** Defines an "extensible bitmap" - a bitmap that can extend to accommodate
    any value.

    It does not currently deal well with a sparse domain - one bit set at 0x1 and
    another at 0x100000 will generate a bitmap with 0x100000/0x8 bytes usage. */
class ExtensibleBitmap
{
    public:
        /** Creates a new, empty bitmap. */
        ExtensibleBitmap();
        /** Creates a new bitmap identical to that given. */
        ExtensibleBitmap(const ExtensibleBitmap &other);
        /** Destroys the bitmap. */
        ~ExtensibleBitmap();

        /** Makes this bitmap mirror the one given. */
        ExtensibleBitmap &operator = (const ExtensibleBitmap &other);

        //
        // Public interface.
        //
        /** Sets the bit in the bitmap indexed by n. */
        void set(size_t n);
        /** Clears the bit in the bitmap indexed by n. */
        void clear(size_t n);
        /** Returns the bit in the bitmap indexed by n. */
        bool test(size_t n);
        /** Returns the index of the first set bit. */
        inline size_t getFirstSet()
        {
            return m_nFirstSetBit;
        }
        /** Returns the index of the first clear bit. */
        inline size_t getFirstClear()
        {
            while(test(m_nFirstClearBit))
            {
                m_nFirstClearBit++;
            }
            return m_nFirstClearBit;
        }
        /** Returns the index of the last set bit. */
        inline size_t getLastSet()
        {
            return m_nLastSetBit;
        }
        /** Returns the index of the last clear bit. */
        inline size_t getLastClear()
        {
            return m_nLastClearBit;
        }

    private:
        /** Performance hint - one statically allocated word. Means we can index
            data from 0..{31,63} without dynamically allocating anything. */
        uintptr_t m_StaticMap;

        /** The dynamic map, to accommodate bit numbers > {31,63} */
        uint8_t *m_pDynamicMap;

        /** Amount of memory the dynamic map occupies. */
        size_t m_DynamicMapSize;

        /** Largest stored bit in the dynamic map. */
        size_t m_nMaxBit;

        /** First/last bit set/clear indexes. */
        size_t m_nFirstSetBit;
        size_t m_nFirstClearBit;
        size_t m_nLastSetBit;
        size_t m_nLastClearBit;
};

#endif
