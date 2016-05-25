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

#include <utilities/ObjectPool.h>

TEST(PedigreeObjectPool, EmptyPoolAllocation)
{
    ObjectPool<int> x;

    int *a = x.allocate();
    int *b = x.allocate();

    EXPECT_NE(a, b);

    delete a;
    delete b;
}

TEST(PedigreeObjectPool, ObjectReuse)
{
    ObjectPool<int, 2> x;

    int *a1 = x.allocate();
    int *b1 = x.allocate();

    x.deallocate(a1);
    x.deallocate(b1);

    int *b2 = x.allocate();
    int *a2 = x.allocate();

    EXPECT_EQ(a1, a2);
    EXPECT_EQ(b1, b2);

    delete a1;
    delete b1;
}

TEST(PedigreeObjectPool, ObjectReuseThenAllocation)
{
    ObjectPool<int, 2> x;

    // Add an object to the pool.
    int *a1 = new int;
    x.deallocate(a1);

    // Re-use that object.
    int *a2 = x.allocate();

    // This will allocate a new object.
    int *a3 = x.allocate();

    EXPECT_EQ(a1, a2);
    EXPECT_NE(a1, a3);

    delete a2;
    delete a3;
}

TEST(PedigreeObjectPool, DISABLED_DeallocatedTooMany)
{
    ObjectPool<int, 1> x;

    // Add a new item to the pool.
    int *a1 = new int;
    x.deallocate(a1);

    // Try to add another item (won't actually add to pool).
    int *b1 = new int;
    x.deallocate(b1);

    // Allocate a new object from the heap in case the current heap
    // implementation gives b1 back to us (which would fail the test).
    int *c = new int;

    // Comes from pool.
    int *a2 = x.allocate();

    // Comes from heap.
    int *b2 = x.allocate();

    EXPECT_EQ(a1, a2);
    EXPECT_NE(b1, b2);

    delete c;
    delete a2;
    delete b2;
}
