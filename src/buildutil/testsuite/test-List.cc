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

#include <utilities/List.h>

struct Foo
{
    Foo() : x(-1)
    {}

    // Must use explicit here to avoid the implicit conversion.
    explicit Foo(int x) : x(x)
    {}

    int x;
};

TEST(PedigreeList, Construction)
{
    List<int> x;
    EXPECT_EQ(x.size(), 0);
    EXPECT_EQ(x.count(), 0);
    EXPECT_EQ(x.begin(), x.end());
}

TEST(PedigreeList, Assignment)
{
    List<int> x;
    x.pushBack(1);

    List<int> y(x);
    EXPECT_EQ(x.size(), y.size());
    EXPECT_EQ(x.count(), y.count());
    EXPECT_EQ(x.popFront(), y.popFront());
}

TEST(PedigreeList, AddItems)
{
    List<int> x;
    x.pushBack(1);
    x.pushBack(5);
    x.pushBack(10);
    EXPECT_EQ(x.size(), 3);
    EXPECT_EQ(x.count(), 3);
    auto it = x.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 5);
    ++it;
    EXPECT_EQ(*it, 10);
}

TEST(PedigreeList, AddRemoveItems)
{
    List<int> x;
    x.pushBack(1);
    x.pushBack(5);
    x.pushBack(10);

    EXPECT_EQ(x.popFront(), 1);
    EXPECT_EQ(x.popFront(), 5);
    EXPECT_EQ(x.popFront(), 10);
    EXPECT_EQ(x.popFront(), 0);
    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeList, AddRemoveItemsReverse)
{
    List<int> x;
    x.pushFront(1);
    x.pushFront(5);
    x.pushFront(10);

    EXPECT_EQ(x.popFront(), 10);
    EXPECT_EQ(x.popFront(), 5);
    EXPECT_EQ(x.popFront(), 1);
    EXPECT_EQ(x.popFront(), 0);
    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeList, PopBack)
{
    List<int> x;
    x.pushFront(1);

    EXPECT_EQ(x.popBack(), 1);

    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeList, PopEmpty)
{
    List<int> x;

    EXPECT_EQ(x.popFront(), 0);
    EXPECT_EQ(x.popBack(), 0);

    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeList, Falseish)
{
    List<int> x;
    x.pushFront(0);

    EXPECT_EQ(x.popFront(), 0);

    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeList, CleanupWorks)
{
    // Expecting a clean Valgrind pass; if Valgrind is not being used then this
    // doesn't really matter very much.
    List<int> x;
    x.pushBack(2);
    x.pushBack(4);

    EXPECT_EQ(x.popBack(), 4);

    EXPECT_EQ(x.count(), 1);
}

TEST(PedigreeList, ErasingBegin)
{
    List<int> x;
    x.pushBack(2);
    x.pushBack(4);

    // Erase takes a reference, we cannot just pass begin() straight in.
    auto it = x.begin();
    it = x.erase(it);
    EXPECT_EQ(*it, 4);
    x.erase(it);

    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeList, ErasingMiddle)
{
    List<int> x;
    x.pushBack(2);
    x.pushBack(4);
    x.pushBack(6);
    x.pushBack(8);

    // Erase takes a reference, we cannot just pass begin() straight in.
    auto it = x.begin();
    ++it;
    it = x.erase(it);
    x.erase(it);

    EXPECT_EQ(x.popFront(), 2);
    EXPECT_EQ(x.popFront(), 8);

    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeList, ErasingEnd)
{
    List<int> x;
    x.pushBack(2);
    x.pushBack(4);
    x.pushBack(6);
    x.pushBack(8);

    auto it = x.begin();
    ++it; ++it; ++it;
    it = x.erase(it);

    EXPECT_EQ(x.popFront(), 2);
    EXPECT_EQ(x.popFront(), 4);
    EXPECT_EQ(x.popFront(), 6);

    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeList, ErasingReverse)
{
    List<int> x;
    x.pushBack(2);
    x.pushBack(4);
    x.pushBack(6);
    x.pushBack(8);

    // Erase takes a reference, we cannot just pass begin() straight in.
    auto it = x.rbegin();
    ++it;
    x.erase(it);

    EXPECT_NE(it, x.rend());

    EXPECT_EQ(x.popFront(), 2);
    EXPECT_EQ(x.popFront(), 4);
    EXPECT_EQ(x.popFront(), 8);

    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeList, Assign)
{
    List<int> x;
    x.pushBack(2);
    x.pushBack(4);
    x.pushBack(6);

    List<int> y;
    y.pushBack(10);  // Will be removed by the assignment.
    y = x;

    EXPECT_EQ(x.popFront(), y.popFront());
    EXPECT_EQ(x.popFront(), y.popFront());
    EXPECT_EQ(x.popFront(), y.popFront());

    EXPECT_EQ(x.count(), y.count());
    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeList, NonZeroableType)
{
    Foo a(2), b(4), c(6);

    List<Foo> x;
    x.pushBack(a);
    x.pushBack(b);
    x.pushBack(c);

    EXPECT_EQ(x.popFront().x, 2);
    EXPECT_EQ(x.popFront().x, 4);
    EXPECT_EQ(x.popFront().x, 6);
    EXPECT_EQ(x.popFront().x, -1);

    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeList, IterateReverse)
{
    List<int> x_;
    x_.pushBack(1);
    x_.pushBack(5);
    x_.pushBack(10);

    const List<int> &x = x_;

    EXPECT_NE(x.rbegin(), x.rend());

    auto it = x.rbegin();
    EXPECT_EQ(*(it++), 10);
    EXPECT_EQ(*(it++), 5);
    EXPECT_EQ(*(it++), 1);
    EXPECT_EQ(it, x.rend());
}

TEST(PedigreeList, PointerPopFront)
{
    List<void *> x;

    void *p = reinterpret_cast<void *>(5);

    x.pushBack(reinterpret_cast<void *>(5));

    EXPECT_EQ(x.popFront(), p);
    EXPECT_EQ(x.popFront(), reinterpret_cast<void *>(0));
}

TEST(PedigreeList, PointerPopBack)
{
    List<void *> x;

    void *p = reinterpret_cast<void *>(5);

    x.pushBack(reinterpret_cast<void *>(5));

    EXPECT_EQ(x.popBack(), p);
    EXPECT_EQ(x.popBack(), reinterpret_cast<void *>(0));
}

TEST(PedigreeList, PointerRotatePop)
{
    List<void *> x;

    void *p = reinterpret_cast<void *>(5);

    x.pushBack(reinterpret_cast<void *>(5));
    void *p_ = x.popFront();
    x.pushBack(p);
    p_ = x.popFront();

    EXPECT_EQ(x.count(), 0);
    EXPECT_EQ(x.popFront(), reinterpret_cast<void *>(0));
    EXPECT_EQ(p, p_);
}
