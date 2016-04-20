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

#include <benchmark/benchmark.h>

#include <utilities/RangeList.h>

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

static void BM_RangeListAllocate(benchmark::State &state)
{
    while (state.KeepRunning())
    {
        state.PauseTiming();
        RangeList<int64_t> list;
        list.free(0, state.range_x());
        state.ResumeTiming();

        int64_t addr;
        for (size_t i = 0; i < state.range_x(); ++i)
        {
            benchmark::DoNotOptimize(list.allocate(1, addr));
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

static void BM_RangeListFree(benchmark::State &state)
{
    while (state.KeepRunning())
    {
        state.PauseTiming();
        RangeList<int64_t> list;
        state.ResumeTiming();

        int64_t addr;
        for (size_t i = 0; i < state.range_x(); ++i)
        {
            list.free(i, 1);
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

static void BM_RangeListScatter(benchmark::State &state)
{
    while (state.KeepRunning())
    {
        state.PauseTiming();
        RangeList<int64_t> list;
        state.ResumeTiming();

        int64_t addr;
        for (size_t i = 0; i < state.range_x(); ++i)
        {
            if (i % 2)
            {
                list.free(i, 10);
            }
            else
            {
                list.allocate(1, addr);
            }
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

BENCHMARK(BM_RangeListAllocate)->Range(1, 2 << 24);
BENCHMARK(BM_RangeListFree)->Range(1, 2 << 24);
BENCHMARK(BM_RangeListScatter)->Range(1, 2 << 24);
