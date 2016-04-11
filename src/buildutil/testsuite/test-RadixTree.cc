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

#include <utilities/RadixTree.h>

TEST(PedigreeRadixTree, Construction)
{
    RadixTree<int> x;
    EXPECT_EQ(x.count(), 0);
    EXPECT_EQ(x.begin(), x.end());
}

TEST(PedigreeRadixTree, CopyConstruction)
{
    RadixTree<int> x;
    x.insert(String("foo"), 1);
    RadixTree<int> y(x);
    EXPECT_EQ(x.count(), y.count());
    EXPECT_EQ(x.lookup(String("foo")), 1);
    EXPECT_EQ(y.lookup(String("foo")), 1);
}

TEST(PedigreeRadixTree, Assignment)
{
    RadixTree<int> x, y;
    x.insert(String("foo"), 1);
    y = x;
    EXPECT_EQ(x.count(), y.count());
    EXPECT_EQ(x.lookup(String("foo")), 1);
    EXPECT_EQ(y.lookup(String("foo")), 1);
}

TEST(PedigreeRadixTree, CaseSensitive)
{
    RadixTree<int> x(true);
    x.insert(String("foo"), 1);
    EXPECT_EQ(x.lookup(String("foo")), 1);
    EXPECT_EQ(x.lookup(String("Foo")), 0);
}

TEST(PedigreeRadixTree, CaseInsensitive)
{
    RadixTree<int> x(false);
    x.insert(String("foo"), 1);
    EXPECT_EQ(x.lookup(String("foo")), 1);
    EXPECT_EQ(x.lookup(String("Foo")), 1);
}

TEST(PedigreeRadixTree, Clear)
{
    RadixTree<int> x;
    x.insert(String("foo"), 1);
    x.insert(String("bar"), 2);
    x.clear();
    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeRadixTree, EmptyLookup)
{
    RadixTree<int> x;
    EXPECT_EQ(x.lookup(String("foo")), 0);
}

TEST(PedigreeRadixTree, EmptyRemove)
{
    RadixTree<int> x;
    x.remove(String("foo"));
    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeRadixTree, EmptyKeyRemove)
{
    RadixTree<int> x;
    x.remove(String());
    EXPECT_EQ(x.count(), 0);
}

TEST(PedigreeRadixTree, PartialMatchMiss)
{
    RadixTree<int> x;
    x.insert(String("foobar"), 1);
    EXPECT_EQ(x.lookup(String("foo")), 0);
}

TEST(PedigreeRadixTree, Removal)
{
    RadixTree<int> x;
    x.insert(String("foo"), 1);
    x.insert(String("bar"), 2);
    x.remove(String("foo"));
    EXPECT_EQ(x.count(), 1);
    EXPECT_EQ(x.lookup(String("foo")), 0);
    EXPECT_EQ(x.lookup(String("bar")), 2);
}

TEST(PedigreeRadixTree, RemovalBigRoot)
{
    RadixTree<int> x;
    x.insert(String("foo"), 1);
    x.insert(String("foobar"), 2);
    x.insert(String("foobaz"), 3);
    x.insert(String("fooqux"), 4);
    x.insert(String("fooabc"), 5);
    x.remove(String("foo"));
    EXPECT_EQ(x.count(), 4);
}

TEST(PedigreeRadixTree, SplitKeys)
{
    RadixTree<int> x;
    x.insert(String("foo"), 1);
    x.insert(String("foobar"), 2);
    x.insert(String("foobarbaz"), 3);
    x.insert(String("foobarbazqux"), 4);
    x.insert(String("foobarbazquux"), 5);
    EXPECT_EQ(x.lookup(String("foo")), 1);
    EXPECT_EQ(x.lookup(String("foobar")), 2);
    EXPECT_EQ(x.lookup(String("foobarbaz")), 3);
    EXPECT_EQ(x.lookup(String("foobarbazqux")), 4);
    EXPECT_EQ(x.lookup(String("foobarbazquux")), 5);
}

TEST(PedigreeRadixTree, SplitKeysBackwards)
{
    RadixTree<int> x;
    x.insert(String("foobarbazquux"), 5);
    x.insert(String("foobarbazqux"), 4);
    x.insert(String("foo"), 1);
    x.insert(String("foobar"), 2);
    x.insert(String("foobarbaz"), 3);
    EXPECT_EQ(x.lookup(String("foo")), 1);
    EXPECT_EQ(x.lookup(String("foobar")), 2);
    EXPECT_EQ(x.lookup(String("foobarbaz")), 3);
    EXPECT_EQ(x.lookup(String("foobarbazqux")), 4);
    EXPECT_EQ(x.lookup(String("foobarbazquux")), 5);
}

TEST(PedigreeRadixTree, Override)
{
    RadixTree<int> x;
    x.insert(String("foo"), 1);
    x.insert(String("foo"), 2);
    EXPECT_EQ(x.lookup(String("foo")), 2);
}

TEST(PedigreeRadixTree, CaseInsensitiveSplitKeys)
{
    RadixTree<int> x(false);
    x.insert(String("foo"), 1);
    x.insert(String("Foobar"), 2);
    EXPECT_EQ(x.lookup(String("foo")), 1);
    EXPECT_EQ(x.lookup(String("Foobar")), 2);
}

TEST(PedigreeRadixTree, CaseInsensitiveBackwardSplitKeys)
{
    RadixTree<int> x(false);
    x.insert(String("Foobar"), 2);
    x.insert(String("foo"), 1);
    EXPECT_EQ(x.lookup(String("foo")), 1);
    EXPECT_EQ(x.lookup(String("Foobar")), 2);
}

TEST(PedigreeRadixTree, Iteration)
{
    RadixTree<int> x;
    x.insert(String("foo"), 1);
    x.insert(String("foobar"), 2);
    x.insert(String("bar"), 3);
    x.insert(String("barfoo"), 4);
    auto it = x.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
}

TEST(PedigreeRadixTree, Erase)
{
    RadixTree<int> x;
    x.insert(String("foo"), 1);
    x.insert(String("foobar"), 2);
    x.insert(String("bar"), 3);
    x.insert(String("barfoo"), 4);

    auto it = x.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    it = x.erase(it);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(x.count(), 3);
}
