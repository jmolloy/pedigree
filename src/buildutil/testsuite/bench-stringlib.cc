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

#include <utilities/utility.h>

static void BM_StringLength(benchmark::State &state)
{
    char *buf = new char[state.range_x()];
    memset(buf, 'a', state.range_x());
    buf[state.range_x() - 1] = '\0';

    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(StringLength(buf));
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));

    delete [] buf;
}

static void BM_StringCopy(benchmark::State &state)
{
    char *buf = new char[state.range_x()];
    memset(buf, 'a', state.range_x());
    buf[state.range_x() - 1] = '\0';
    char *dest = new char[state.range_x()];

    while (state.KeepRunning())
    {
        StringCopy(dest, buf);
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));

    delete [] dest;
    delete [] buf;
}

static void BM_StringCopyN(benchmark::State &state)
{
    char *buf = new char[state.range_x()];
    memset(buf, 'a', state.range_x());
    buf[state.range_x() - 1] = '\0';
    char *dest = new char[state.range_x()];

    while (state.KeepRunning())
    {
        StringCopyN(dest, buf, state.range_x());
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));

    delete [] dest;
    delete [] buf;
}

static void BM_StringCompare(benchmark::State &state)
{
    char *buf1 = new char[state.range_x()];
    memset(buf1, 'a', state.range_x());
    buf1[state.range_x() - 1] = '\0';
    char *buf2 = new char[state.range_x()];

    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(StringCompare(buf1, buf2));
        ++(*buf1);
        ++(*buf2);
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));

    delete [] buf2;
    delete [] buf1;
}

static void BM_StringCompareN(benchmark::State &state)
{
    char *buf1 = new char[state.range_x()];
    memset(buf1, 'a', state.range_x());
    buf1[state.range_x() - 1] = '\0';
    char *buf2 = new char[state.range_x()];

    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(StringCopyN(buf1, buf2, state.range_x()));
        ++(*buf1);
        ++(*buf2);
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));

    delete [] buf2;
    delete [] buf1;
}

BENCHMARK(BM_StringLength)->Range(8, 8<<16);
BENCHMARK(BM_StringCopy)->Range(8, 8<<16);
BENCHMARK(BM_StringCopyN)->Range(8, 8<<16);
