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

#include <gtest/gtest.h>

#include <utilities/ExtensibleBitmap.h>

TEST(PedigreeExtensibleBitmap, InitialState)
{
    ExtensibleBitmap bitmap;

    EXPECT_EQ(bitmap.getFirstSet(), ~0U);
    EXPECT_EQ(bitmap.getFirstClear(), 0);
    EXPECT_EQ(bitmap.getLastSet(), ~0U);
    EXPECT_EQ(bitmap.getLastClear(), 0);
}

TEST(PedigreeExtensibleBitmap, CopyConstruct)
{
    ExtensibleBitmap bitmap;
    bitmap.set(1);
    bitmap.set(128);

    ExtensibleBitmap bitmap2(bitmap);

    EXPECT_FALSE(bitmap2.test(0));
    EXPECT_TRUE(bitmap2.test(1));
    EXPECT_TRUE(bitmap2.test(128));
}

TEST(PedigreeExtensibleBitmap, Assignment)
{
    ExtensibleBitmap bitmap;
    bitmap.set(1);
    bitmap.set(128);

    ExtensibleBitmap bitmap2;
    bitmap2 = bitmap;

    EXPECT_FALSE(bitmap2.test(0));
    EXPECT_TRUE(bitmap2.test(1));
    EXPECT_TRUE(bitmap2.test(128));
}

TEST(PedigreeExtensibleBitmap, SetOne)
{
    ExtensibleBitmap bitmap;

    bitmap.set(0);

    EXPECT_TRUE(bitmap.test(0));
    EXPECT_FALSE(bitmap.test(1));
    EXPECT_EQ(bitmap.getFirstSet(), 0);
    EXPECT_EQ(bitmap.getFirstClear(), 1);
    EXPECT_EQ(bitmap.getLastSet(), 0);
    EXPECT_EQ(bitmap.getLastClear(), 0);
}

TEST(PedigreeExtensibleBitmap, SetThenClear)
{
    ExtensibleBitmap bitmap;

    bitmap.set(0);
    bitmap.clear(0);

    EXPECT_FALSE(bitmap.test(0));
    EXPECT_FALSE(bitmap.test(1));
    EXPECT_EQ(bitmap.getFirstSet(), ~0U);
    EXPECT_EQ(bitmap.getFirstClear(), 0);
    EXPECT_EQ(bitmap.getLastSet(), ~0U);
    EXPECT_EQ(bitmap.getLastClear(), 0);
}

TEST(PedigreeExtensibleBitmap, SetMultiple)
{
    ExtensibleBitmap bitmap;

    for (size_t i = 0; i < 5; ++i)
    {
        bitmap.set(i);
    }

    EXPECT_TRUE(bitmap.test(0));
    EXPECT_TRUE(bitmap.test(1));
    EXPECT_TRUE(bitmap.test(2));
    EXPECT_TRUE(bitmap.test(3));
    EXPECT_TRUE(bitmap.test(4));
    EXPECT_FALSE(bitmap.test(5));
    EXPECT_EQ(bitmap.getFirstSet(), 0);
    EXPECT_EQ(bitmap.getFirstClear(), 5);
    EXPECT_EQ(bitmap.getLastSet(), 4);
    EXPECT_EQ(bitmap.getLastClear(), 0);
}

TEST(PedigreeExtensibleBitmap, SetDynamic)
{
    ExtensibleBitmap bitmap;

    // Initial add of dynamic memory.
    bitmap.set(128);

    // Resize dynamic bitmap.
    bitmap.set(256);

    EXPECT_TRUE(bitmap.test(128));
    EXPECT_TRUE(bitmap.test(256));
    EXPECT_FALSE(bitmap.test(0));
    EXPECT_FALSE(bitmap.test(127));
    EXPECT_FALSE(bitmap.test(129));
    EXPECT_EQ(bitmap.getFirstSet(), 128);
    EXPECT_EQ(bitmap.getFirstClear(), 0);
    EXPECT_EQ(bitmap.getLastSet(), 256);
    EXPECT_EQ(bitmap.getLastClear(), 0);
}

TEST(PedigreeExtensibleBitmap, ClearDynamic)
{
    ExtensibleBitmap bitmap;

    bitmap.set(128);
    bitmap.set(256);

    bitmap.clear(128);

    EXPECT_TRUE(bitmap.test(256));
    EXPECT_FALSE(bitmap.test(128));
}

TEST(PedigreeExtensibleBitmap, ClearEdge)
{
    ExtensibleBitmap bitmap;

    /// \todo rewrite this to use sizeof(uintptr_t)

    bitmap.set(0x44);
    bitmap.clear(0x44);

    bitmap.clear(0x40);

    EXPECT_FALSE(bitmap.test(0x44));
    EXPECT_FALSE(bitmap.test(0x40));
}
