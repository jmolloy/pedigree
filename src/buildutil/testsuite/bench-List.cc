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

#include <benchmark/benchmark.h>

#include <utilities/List.h>

static void BM_ListPushBack(benchmark::State &state)
{
    List<int64_t> list;
    const int64_t value = 1;

    while (state.KeepRunning())
    {
        state.PauseTiming();
        list.clear();
        state.ResumeTiming();

        for (size_t i = 0; i < state.range_x(); ++i)
        {
            list.pushBack(value);
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

static void BM_ListPushFront(benchmark::State &state)
{
    List<int64_t> list;
    const int64_t value = 1;

    while (state.KeepRunning())
    {
        state.PauseTiming();
        list.clear();
        state.ResumeTiming();

        for (size_t i = 0; i < state.range_x(); ++i)
        {
            list.pushFront(value);
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

static void BM_ListPopFront(benchmark::State &state)
{
    List<int64_t> list;
    const int64_t value = 1;

    while (state.KeepRunning())
    {
        state.PauseTiming();
        list.clear();
        for (size_t i = 0; i < state.range_x(); ++i)
        {
            list.pushFront(value);
        }
        state.ResumeTiming();

        for (size_t i = 0; i < state.range_x(); ++i)
        {
            list.popFront();
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

static void BM_ListPopBack(benchmark::State &state)
{
    List<int64_t> list;
    const int64_t value = 1;

    while (state.KeepRunning())
    {
        state.PauseTiming();
        list.clear();
        for (size_t i = 0; i < state.range_x(); ++i)
        {
            list.pushFront(value);
        }
        state.ResumeTiming();

        for (size_t i = 0; i < state.range_x(); ++i)
        {
            list.popBack();
        }
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));
}

BENCHMARK(BM_ListPushFront)->Range(8, 8<<16);
BENCHMARK(BM_ListPushBack)->Range(8, 8<<16);
BENCHMARK(BM_ListPopFront)->Range(8, 8<<16);
BENCHMARK(BM_ListPopBack)->Range(8, 8<<16);
