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

#include <stdlib.h>
#include <time.h>

#include <benchmark/benchmark.h>

#include <utilities/Tree.h>

#define RANDOM_MAX 0x10000

static const int RandomNumber()
{
    static bool seeded = false;
    if (!seeded)
    {
        srand(time(0));
        seeded = true;
    }

    // Artificially limit the random number range so we get collisions.
    return rand() % RANDOM_MAX;
}

static void BM_TreeInsert(benchmark::State &state)
{
    Tree<int64_t, int64_t> tree;
    const int64_t value = 1;

    while (state.KeepRunning())
    {
        state.PauseTiming();
        tree.clear();
        state.ResumeTiming();

        for (size_t i = 0; i < state.range_x(); ++i)
        {
            tree.insert(i, value);
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

static void BM_TreeRandomlyInsert(benchmark::State &state)
{
    Tree<int64_t, int64_t> tree;
    const int64_t value = 1;

    while (state.KeepRunning())
    {
        state.PauseTiming();
        tree.clear();
        state.ResumeTiming();

        for (size_t i = 0; i < state.range_x(); ++i)
        {
            tree.insert(RandomNumber(), value);
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

static void BM_TreeLookup(benchmark::State &state)
{
    Tree<int64_t, int64_t> tree;

    for (size_t i = 0; i < state.range_x(); ++i)
    {
        tree.insert(RandomNumber(), RandomNumber());
    }

    while (state.KeepRunning())
    {
        for (size_t i = 0; i < state.range_y(); ++i)
        {
            benchmark::DoNotOptimize(tree.lookup(RandomNumber()));
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_y()));
}

BENCHMARK(BM_TreeInsert)->Range(8, 8<<16);
BENCHMARK(BM_TreeRandomlyInsert)->Range(8, 8<<16);
BENCHMARK(BM_TreeLookup)->RangePair(8, 8<<16, 8, 8<<16);
