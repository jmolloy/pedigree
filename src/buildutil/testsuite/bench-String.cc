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

#include <utilities/String.h>

static void BM_CxxStringCreation(benchmark::State &state)
{
    while (state.KeepRunning())
    {
        String s;
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

static void BM_CxxStringCopyToStatic(benchmark::State &state)
{
    const char *assign = "Hello, world!";

    while (state.KeepRunning())
    {
        String s(assign);
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

static void BM_CxxStringCopyToDynamic(benchmark::State &state)
{
    char assign[128];
    memset(assign, 'a', 128);
    assign[127] = 0;

    while (state.KeepRunning())
    {
        String s(assign);
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

static void BM_CxxStringFormat(benchmark::State &state)
{
    while (state.KeepRunning())
    {
        String s;
        s.Format("Hello, %s!", "world");
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

static void BM_CxxStringStartswith(benchmark::State &state)
{
    String s("hello, world!");

    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(s.startswith("hello"));
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

static void BM_CxxStringEndswith(benchmark::State &state)
{
    String s("hello, world!");

    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(s.endswith("world!"));
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

static void BM_CxxStringStrip(benchmark::State &state)
{
    char buf[4096];
    memset(buf, ' ', 4096);
    buf[2048] = 'a';
    buf[4095] = 0;

    while (state.KeepRunning())
    {
        state.PauseTiming();
        String s(buf);
        state.ResumeTiming();

        s.strip();
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

static void BM_CxxStringSplit(benchmark::State &state)
{
    char *buf = new char[state.range_x()];
    memset(buf, 'a', state.range_x());
    buf[state.range_x()] = 0;

    while (state.KeepRunning())
    {
        state.PauseTiming();
        String s(buf);
        state.ResumeTiming();

        benchmark::DoNotOptimize(s.split(state.range_x() / 2));
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

static void BM_CxxStringTokenize(benchmark::State &state)
{
    char buf[4096];
    memset(buf, 0, 4096);
    for (size_t i = 0; i < 4095; ++i)
    {
        if (i % 2)
        {
            buf[i] = ' ';
        }
        else
        {
            buf[i] = 'a';
        }
    }

    while (state.KeepRunning())
    {
        state.PauseTiming();
        String s(buf);
        state.ResumeTiming();

        benchmark::DoNotOptimize(s.tokenise(' '));
    }

    state.SetItemsProcessed(int64_t(state.iterations()));
}

BENCHMARK(BM_CxxStringCreation);
BENCHMARK(BM_CxxStringCopyToStatic);
BENCHMARK(BM_CxxStringCopyToDynamic);
BENCHMARK(BM_CxxStringFormat);
BENCHMARK(BM_CxxStringStartswith);
BENCHMARK(BM_CxxStringEndswith);
BENCHMARK(BM_CxxStringStrip);
BENCHMARK(BM_CxxStringSplit)->Range(8, 8<<8);
BENCHMARK(BM_CxxStringTokenize);
