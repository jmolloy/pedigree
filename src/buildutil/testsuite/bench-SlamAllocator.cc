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

#include <lib/SlamAllocator.h>

static void BM_SlamAllocatorBackForth(benchmark::State &state)
{
    while (state.KeepRunning())
    {
        uintptr_t mem = SlamAllocator::instance().allocate(OBJECT_MINIMUM_SIZE);
        SlamAllocator::instance().free(mem);
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
    state.SetBytesProcessed(int64_t(state.iterations()) * OBJECT_MINIMUM_SIZE);
}

static void BM_SlamAllocatorAllocations(benchmark::State &state)
{
    SlamSupport::unmapAll();


    while (state.KeepRunning())
    {
        state.PauseTiming();
        SlamAllocator *allocator = new SlamAllocator();
        allocator->initialise();
        state.ResumeTiming();

        for (size_t i = 0; i < state.range_y(); ++i)
        {
            allocator->allocate(state.range_x());
        }

        state.PauseTiming();
        delete allocator;
        SlamSupport::unmapAll();
        state.ResumeTiming();
    }

    int64_t items = int64_t(state.iterations()) * int64_t(state.range_y());
    state.SetItemsProcessed(items);
    state.SetBytesProcessed(items * int64_t(state.range_x()));
}

BENCHMARK(BM_SlamAllocatorBackForth);
BENCHMARK(BM_SlamAllocatorAllocations)->RangePair(OBJECT_MINIMUM_SIZE, 4096, 8, 1024);
