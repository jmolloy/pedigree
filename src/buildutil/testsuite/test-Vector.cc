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

#include <utilities/Vector.h>

TEST(PedigreeVector, Construction)
{
    Vector<int> x;
    EXPECT_EQ(x.size(), 0);
    EXPECT_EQ(x.count(), 0);
    EXPECT_EQ(x.begin(), x.end());
}

TEST(PedigreeVector, ConstructionReserve)
{
    Vector<int> x(5);
    EXPECT_EQ(x.size(), 5);
    EXPECT_EQ(x.count(), 0);
    EXPECT_EQ(x.begin(), x.end());
}

TEST(PedigreeVector, Assignment)
{
    Vector<int> x;
    x.pushBack(5);
    Vector<int> y(x);
    EXPECT_EQ(x.size(), y.size());
    EXPECT_EQ(x.count(), y.count());
    EXPECT_EQ(x.popFront(), y.popFront());
}

TEST(PedigreeVector, AssignmentOperator)
{
    Vector<int> x, y;
    x.pushBack(5);
    y = x;
    EXPECT_EQ(x.size(), y.size());
    EXPECT_EQ(x.count(), y.count());
    EXPECT_EQ(x.popFront(), y.popFront());
}

TEST(PedigreeVector, Indexing)
{
    Vector<int> x;
    x.pushBack(1);
    x.pushBack(2);
    x.pushBack(3);
    EXPECT_EQ(x[0], 1);
    EXPECT_EQ(x[1], 2);
    EXPECT_EQ(x[2], 3);
}

TEST(PedigreeVector, IndexingTooFar)
{
    Vector<int> x;
    x.pushBack(1);
    EXPECT_EQ(x[5], 0);
}

TEST(PedigreeVector, Insertion)
{
    Vector<int> x;
    x.pushFront(1);
    x.pushFront(2);
    x.pushFront(3);
    EXPECT_EQ(x[2], 1);
    EXPECT_EQ(x[1], 2);
    EXPECT_EQ(x[0], 3);
}

TEST(PedigreeVector, SetAt)
{
    Vector<int> x;
    x.pushBack(1);
    x.pushBack(2);
    x.pushBack(3);
    x.setAt(1, 5);
    EXPECT_EQ(x[0], 1);
    EXPECT_EQ(x[1], 5);
    EXPECT_EQ(x[2], 3);
}

TEST(PedigreeVector, Clear)
{
    Vector<int> x;
    x.pushBack(1);
    x.pushBack(2);
    x.pushBack(3);
    EXPECT_EQ(x.count(), 3);
    x.clear();
    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeVector, ForwardIterate)
{
    Vector<int> x;
    x.pushBack(1);
    x.pushBack(2);
    x.pushBack(3);

    const Vector<int> &y = x;
    EXPECT_NE(y.begin(), y.end());
    auto it = y.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(it, y.end());
}

TEST(PedigreeVector, PopBack)
{
    Vector<int> x;
    x.pushBack(1);
    x.pushBack(2);
    x.pushBack(3);
    EXPECT_EQ(x.popBack(), 3);
    EXPECT_EQ(x.popBack(), 2);
    EXPECT_EQ(x.popBack(), 1);
    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeVector, Erase)
{
    Vector<int> x;
    x.pushBack(1);
    x.pushBack(2);
    x.pushBack(3);

    auto it = x.begin();
    ++it;

    x.erase(it);

    EXPECT_EQ(x[0], 1);
    EXPECT_EQ(x[1], 3);
    EXPECT_EQ(x.count(), 2);
}
