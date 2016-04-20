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

#define PEDIGREE_EXTERNAL_SOURCE 1

#include <string.h>

#include <gtest/gtest.h>

#include <utilities/utility.h>

TEST(PedigreeMemoryLibrary, ForwardMemoryCopy)
{
    char buf1[16];
    char buf2[16];

    memset(buf1, 0, 16);
    memset(buf2, 0xAB, 16);

    MemoryCopy(buf1, buf2, 16);

    EXPECT_EQ(memcmp(buf1, buf2, 16), 0);
}

TEST(PedigreeMemoryLibrary, OverlappingMemoryCopy)
{
    char buf[16] = {0};
    char expected[16] = {0};

    // This copy will bring across the null terminating byte.
    strcpy(buf, "ABCDEFGHIJKLMNO");
    strcpy(expected, "BCDEFGHIJKLMNO");

    MemoryCopy(buf, buf + 1, 16);

    EXPECT_STREQ(buf, expected);
}

TEST(PedigreeMemoryLibrary, ReversedMemoryCopy)
{
    char buf[32] = {0};
    char expected[32] = {0};

    // This copy will bring across the null terminating byte.
    strcpy(buf, "ABCDEFGHIJKLMNO");
    strcpy(expected, "AABCDEFGHIJKLMNO");

    MemoryCopy(buf + 1, buf, 16);

    EXPECT_STREQ(buf, expected);
}

TEST(PedigreeMemoryLibrary, MemoryCompareSame)
{
    char buf1[32];
    char buf2[32];

    memset(buf1, 'a', 32);
    memset(buf2, 'a', 32);

    EXPECT_EQ(MemoryCompare(buf1, buf2, 32), 0);
}

TEST(PedigreeMemoryLibrary, MemoryCompareDiffers)
{
    char buf1[32];
    char buf2[32];

    memset(buf1, 'a', 32);
    memset(buf2, 'b', 32);

    EXPECT_NE(MemoryCompare(buf1, buf2, 32), 0);
}

TEST(PedigreeMemoryLibrary, MemoryCompareHalfDiffers)
{
    char buf1[32];
    char buf2[32];

    memset(buf1, 'a', 32);
    memset(buf2, 'a', 16);
    memset(buf2 + 16, 'b', 16);

    EXPECT_NE(MemoryCompare(buf1, buf2, 32), 0);
}
