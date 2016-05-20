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

#include <utilities/HashTable.h>

template <int hashModulo = 0>
class HashableIntegerBase
{
    public:
        HashableIntegerBase() : x_(-1)
        {
        }

        HashableIntegerBase(int x) : x_(x)
        {
        }

        virtual ~HashableIntegerBase()
        {
        }

        virtual size_t hash() const
        {
            if (hashModulo)
            {
                return x_ % hashModulo;
            }
            else
            {
                return x_;
            }
        }

        virtual bool operator == (const HashableIntegerBase &other) const
        {
            return x_ == other.x_;
        }

        virtual bool operator != (const HashableIntegerBase &other) const
        {
            return x_ != other.x_;
        }

    private:
        int x_;
};

class CollidingHashableInteger : public HashableIntegerBase<0>
{
    public:
        CollidingHashableInteger() : HashableIntegerBase()
        {
        }

        CollidingHashableInteger(int x) : HashableIntegerBase(x)
        {
        }

        size_t hash() const
        {
            return 1;
        }
};

class HashableInteger : public HashableIntegerBase<0>
{
    public:
        HashableInteger() : HashableIntegerBase()
        {
        }

        HashableInteger(int x) : HashableIntegerBase(x)
        {
        }
};

class ModuloTenHashableInteger : public HashableIntegerBase<10>
{
    public:
        ModuloTenHashableInteger() : HashableIntegerBase()
        {
        }

        ModuloTenHashableInteger(int x) : HashableIntegerBase(x)
        {
        }
};

TEST(PedigreeHashTable, NoOpRemoval)
{
    HashTable<HashableInteger, int> hashtable;

    HashableInteger key(0);
    EXPECT_EQ(hashtable.lookup(key), (int *) 0);

    hashtable.remove(key);

    EXPECT_EQ(hashtable.lookup(key), (int *) 0);
}

TEST(PedigreeHashTable, RemoveImpossibleHash)
{
    HashTable<HashableInteger, int> hashtable(5);

    HashableInteger key(50);
    EXPECT_EQ(hashtable.lookup(key), (int *) 0);

    hashtable.remove(key);

    EXPECT_EQ(hashtable.lookup(key), (int *) 0);
}

TEST(PedigreeHashTable, AnotherNoOpRemoval)
{
    HashTable<HashableInteger, int> hashtable(5);

    HashableInteger key(3);
    EXPECT_EQ(hashtable.lookup(key), (int *) 0);

    hashtable.remove(key);

    EXPECT_EQ(hashtable.lookup(key), (int *) 0);
}

TEST(PedigreeHashTable, RemoveInserted)
{
    HashTable<HashableInteger, int> hashtable(5);

    int *value = new int;

    HashableInteger key(3);
    hashtable.insert(key, value);

    EXPECT_EQ(hashtable.lookup(key), value);

    hashtable.remove(key);

    EXPECT_EQ(hashtable.lookup(key), (int *) 0);

    delete value;
}

TEST(PedigreeHashTable, NoBuckets)
{
    HashTable<HashableInteger, int> hashtable;

    int *value = new int;

    HashableInteger key(0);

    hashtable.insert(key, value);
    EXPECT_EQ(hashtable.lookup(key), (int *) 0);

    delete value;
}

TEST(PedigreeHashTable, InsertedAlready)
{
    HashTable<HashableInteger, int> hashtable(10);

    int *value1 = new int;
    int *value2 = new int;

    HashableInteger key(0);

    hashtable.insert(key, value1);
    hashtable.insert(key, value2);
    EXPECT_EQ(hashtable.lookup(key), value1);

    delete value1;
    delete value2;
}

TEST(PedigreeHashTable, CollidingHashes)
{
    HashTable<CollidingHashableInteger, int> hashtable(10);

    int *value1 = new int;
    int *value2 = new int;

    CollidingHashableInteger key1(0), key2(1);

    hashtable.insert(key1, value1);
    hashtable.insert(key2, value2);
    EXPECT_EQ(hashtable.lookup(key1), value1);
    EXPECT_EQ(hashtable.lookup(key2), value2);

    delete value1;
    delete value2;
}

TEST(PedigreeHashTable, InsertionNoChains)
{
    HashTable<HashableInteger, int> hashtable(10);

    int *value = new int;

    for (size_t i = 0; i < 10; ++i)
    {
        HashableInteger key(i);
        hashtable.insert(key, value + i);
    }

    for (size_t i = 0; i < 10; ++i)
    {
        HashableInteger key(i);
        EXPECT_EQ(hashtable.lookup(key), value + i);
    }

    delete value;
}

TEST(PedigreeHashTable, InsertionWithChains)
{
    HashTable<ModuloTenHashableInteger, int> hashtable(10);

    int *value = new int;

    for (size_t i = 0; i < 20; ++i)
    {
        ModuloTenHashableInteger key(i);
        hashtable.insert(key, value + i);
    }

    for (size_t i = 0; i < 20; ++i)
    {
        ModuloTenHashableInteger key(i);
        EXPECT_EQ(hashtable.lookup(key), value + i);
    }

    delete value;
}

TEST(PedigreeHashTable, RemoveChained)
{
    HashTable<CollidingHashableInteger, int> hashtable(10);

    int *value1 = new int;
    int *value2 = new int;
    int *value3 = new int;

    CollidingHashableInteger key1(0), key2(1), key3(2);

    hashtable.insert(key1, value1);
    hashtable.insert(key2, value2);
    hashtable.insert(key3, value3);

    hashtable.remove(key2);

    EXPECT_EQ(hashtable.lookup(key1), value1);
    EXPECT_EQ(hashtable.lookup(key2), (int *) 0);
    EXPECT_EQ(hashtable.lookup(key3), value3);

    delete value1;
    delete value2;
    delete value3;
}

TEST(PedigreeHashTable, RemoveFirstInChain)
{
    HashTable<CollidingHashableInteger, int> hashtable(10);

    int *value1 = new int;
    int *value2 = new int;

    CollidingHashableInteger key1(0), key2(1);

    hashtable.insert(key1, value1);
    hashtable.insert(key2, value2);

    hashtable.remove(key1);

    EXPECT_EQ(hashtable.lookup(key1), (int *) 0);
    EXPECT_EQ(hashtable.lookup(key2), value2);

    delete value1;
    delete value2;
}
