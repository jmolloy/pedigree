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

#include <iostream>

#include <gtest/gtest.h>

#include <utilities/String.h>

TEST(PedigreeSharedPointer, Construction)
{
    SharedPointer<int> p;
    EXPECT_FALSE(p);
}

TEST(PedigreeSharedPointer, TakeOwnership)
{
    // Should not provide a hit when run under Valgrind, either.
    SharedPointer<int> p(new int);
    EXPECT_TRUE(p);
}

TEST(PedigreeSharedPointer, CopyOwnership)
{
    // Should not provide a hit when run under Valgrind, either.
    SharedPointer<int> p(new int);
    SharedPointer<int> p2(p);
    EXPECT_TRUE(p);
    EXPECT_TRUE(p2);
}

TEST(PedigreeSharedPointer, Reset)
{
    SharedPointer<int> p(new int);
    EXPECT_TRUE(p);
    p.reset();
    EXPECT_FALSE(p);
}

TEST(PedigreeSharedPointer, ResetNewPointer)
{
    int *a = new int;
    int *b = new int;
    SharedPointer<int> p(a);
    EXPECT_TRUE(p);
    p.reset(b);
    EXPECT_TRUE(p);
    EXPECT_NE(p.get(), a);
}

TEST(PedigreeSharedPointer, GetNotHeld)
{
    SharedPointer<int> p;
    EXPECT_EQ(p.get(), reinterpret_cast<int *>(0));
}

TEST(PedigreeSharedPointer, Deref)
{
    SharedPointer<int> p(new int(5));
    EXPECT_EQ(*p, 5);
}

TEST(PedigreeSharedPointer, Uniqueness)
{
    SharedPointer<int> p(new int);
    EXPECT_TRUE(p.unique());
    SharedPointer<int> p2(p);
    EXPECT_FALSE(p.unique());
    EXPECT_FALSE(p2.unique());
}

TEST(PedigreeSharedPointer, Allocated)
{
    SharedPointer<int> p = SharedPointer<int>::allocate();
    EXPECT_TRUE(p);
}

TEST(PedigreeSharedPointer, Refcount)
{
    SharedPointer<int> p;
    EXPECT_EQ(p.refcount(), 0);
    p.reset(new int);
    EXPECT_EQ(p.refcount(), 1);
}
