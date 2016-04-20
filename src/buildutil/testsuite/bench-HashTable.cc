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

#include <vector>

#include <benchmark/benchmark.h>

#include <utilities/HashTable.h>

class HashedInteger
{
    public:
        HashedInteger() : n_(-1)
        {
        }

        HashedInteger(int n) : n_(n)
        {
        }

        int hash() const
        {
            return n_;
        }

        bool operator == (HashedInteger &other) const
        {
            return n_ == other.n_;
        }

        bool operator != (HashedInteger &other) const
        {
            return n_ != other.n_;
        }

    private:
        int64_t n_;
};

class CollidingHashedInteger
{
    public:
        CollidingHashedInteger() : modulus_(0xFFFFFFFFU), n_(-1)
        {
        }

        CollidingHashedInteger(int modulus, int n) : modulus_(modulus), n_(n)
        {
        }

        int hash() const
        {
            return n_ % modulus_;
        }

        bool operator == (CollidingHashedInteger &other) const
        {
            return n_ == other.n_;
        }

        bool operator != (CollidingHashedInteger &other) const
        {
            return n_ != other.n_;
        }

    private:
        int64_t n_;
        int64_t modulus_;
};

static void BM_HashTableInsertPreallocate(benchmark::State &state)
{
    int64_t value = 1;

    while (state.KeepRunning())
    {
        state.PauseTiming();
        HashTable<HashedInteger, int64_t> table;
        table.initialise(state.range_x(), true);
        state.ResumeTiming();

        for (size_t i = 0; i < state.range_x(); ++i)
        {
            HashedInteger key(i);
            table.insert(key, &value);
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

static void BM_HashTableInsertNoChains(benchmark::State &state)
{
    int64_t value = 1;

    while (state.KeepRunning())
    {
        state.PauseTiming();
        HashTable<HashedInteger, int64_t> table(state.range_x());
        state.ResumeTiming();

        for (size_t i = 0; i < state.range_x(); ++i)
        {
            HashedInteger key(i);
            table.insert(key, &value);
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

static void BM_HashTableLookupNoChains(benchmark::State &state)
{
    HashTable<HashedInteger, int64_t> table(state.range_x());
    int64_t value = 1;
    for (size_t i = 0; i < state.range_x(); ++i)
    {
        HashedInteger key(i);
        table.insert(key, &value);
    }

    while (state.KeepRunning())
    {
        for (size_t i = 0; i < state.range_x(); ++i)
        {
            HashedInteger key(i);
            benchmark::DoNotOptimize(table.lookup(key));
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

static void BM_HashTableInsertWithChains(benchmark::State &state)
{
    int64_t value = 1;

    while (state.KeepRunning())
    {
        state.PauseTiming();
        HashTable<CollidingHashedInteger, int64_t> table(state.range_x());
        state.ResumeTiming();

        for (size_t i = 0; i < state.range_x(); ++i)
        {
            CollidingHashedInteger key(state.range_x() / 2, i);
            table.insert(key, &value);
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

static void BM_HashTableLookupWithChains(benchmark::State &state)
{
    HashTable<CollidingHashedInteger, int64_t> table(state.range_x());
    int64_t value = 1;
    for (size_t i = 0; i < state.range_x(); ++i)
    {
        CollidingHashedInteger key(state.range_x() / 2, i);
        table.insert(key, &value);
    }

    while (state.KeepRunning())
    {
        for (size_t i = 0; i < state.range_x(); ++i)
        {
            CollidingHashedInteger key(state.range_x() / 2, i);
            benchmark::DoNotOptimize(table.lookup(key));
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

BENCHMARK(BM_HashTableInsertPreallocate)->Range(8, 16384);
BENCHMARK(BM_HashTableInsertNoChains)->Range(8, 16384);
BENCHMARK(BM_HashTableLookupNoChains)->Range(8, 16384);
BENCHMARK(BM_HashTableInsertWithChains)->Range(8, 16384);
BENCHMARK(BM_HashTableLookupWithChains)->Range(8, 16384);
