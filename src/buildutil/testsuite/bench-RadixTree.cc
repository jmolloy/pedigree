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

#include <utilities/RadixTree.h>

#define RANDOM_MAX 0x1000000

static const int RandomNumber(const int maximum = RANDOM_MAX)
{
    static bool seeded = false;
    if (!seeded)
    {
        srand(time(0));
        seeded = true;
    }

    // Artificially limit the random number range so we get collisions.
    return rand() % maximum;
}

static void BM_RadixTreeInsert(benchmark::State &state)
{
    RadixTree<int64_t> tree;
    const int64_t value = 1;
    int64_t key_n = 0;

    String key;
    while (state.KeepRunning())
    {
        state.PauseTiming();
        key.Format("%d", key_n++);
        state.ResumeTiming();
        tree.insert(key, value);
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

static void BM_RadixTreeInsertSame(benchmark::State &state)
{
    RadixTree<int64_t> tree;
    const int64_t value = 1;

    String key("key");

    while (state.KeepRunning())
    {
        tree.insert(key, value);
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

static void BM_RadixTreeLookupHit(benchmark::State &state)
{
    // NOTE: all of these will hit in the RadixTree.

    RadixTree<int64_t> tree;
    const int64_t value = 1;

    // Fill the RadixTree for the lookup iterations.
    for (size_t i = 0; i < state.range_x(); ++i)
    {
        String key;
        key.Format("%d", RandomNumber());
        tree.insert(key, value);
    }

    String key;
    while (state.KeepRunning())
    {
        state.PauseTiming();
        key.Format("%d", RandomNumber(state.range_x()));
        state.ResumeTiming();
        benchmark::DoNotOptimize(tree.lookup(key));
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

static void BM_RadixTreeLookupMiss(benchmark::State &state)
{
    // NOTE: all of these will NOT hit in the RadixTree.

    RadixTree<int64_t> tree;
    const int64_t value = 1;

    // Fill the RadixTree for the lookup iterations.
    for (size_t i = 0; i < state.range_x(); ++i)
    {
        String key;
        key.Format("%d", RandomNumber());
        tree.insert(key, value);
    }

    String key;
    while (state.KeepRunning())
    {
        state.PauseTiming();
        // We adjust the key so we'll never get a hit, but every miss is not
        // optimal (e.g. end of string).
        key.Format("%d_", RandomNumber(state.range_x()));
        state.ResumeTiming();
        benchmark::DoNotOptimize(tree.lookup(key));
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

BENCHMARK(BM_RadixTreeInsert);
BENCHMARK(BM_RadixTreeInsertSame);
BENCHMARK(BM_RadixTreeLookupHit)->Range(8, 2048);
BENCHMARK(BM_RadixTreeLookupMiss)->Range(8, 2048);
