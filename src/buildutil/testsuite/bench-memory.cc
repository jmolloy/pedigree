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

static void BM_Memory_MemoryCopy(benchmark::State &state)
{
    char *src = new char[state.range_x()];
    char *dest = new char[state.range_x()];
    memset(src, 'a', state.range_x());

    while (state.KeepRunning())
    {
        MemoryCopy(dest, src, state.range_x());
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));

    delete [] dest;
    delete [] src;
}

static void BM_Memory_OverlappedMemoryCopy(benchmark::State &state)
{
    char *buf = new char[state.range_x()];
    memset(buf, 'a', state.range_x());

    while (state.KeepRunning())
    {
        MemoryCopy(buf + 1, buf, state.range_x() - 1);
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));

    delete [] buf;
}

static void BM_Memory_ForwardMemoryCopy(benchmark::State &state)
{
    char *src = new char[state.range_x()];
    char *dest = new char[state.range_x()];
    memset(src, 'a', state.range_x());

    while (state.KeepRunning())
    {
        ForwardMemoryCopy(dest, src, state.range_x());
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));

    delete [] dest;
    delete [] src;
}

static void BM_Memory_ByteSet(benchmark::State &state)
{
    char *buf = new char[state.range_x()];

    while (state.KeepRunning())
    {
        ByteSet(buf, 0xAB, state.range_x());
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));

    delete [] buf;
}

static void BM_Memory_ByteSetZero(benchmark::State &state)
{
    char *buf = new char[state.range_x()];

    while (state.KeepRunning())
    {
        ByteSet(buf, 0, state.range_x());
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));

    delete [] buf;
}

static void BM_Memory_WordSet(benchmark::State &state)
{
    const int factor = 2;
    char *buf = new char[state.range_x()];

    while (state.KeepRunning())
    {
        WordSet(buf, 0xAB, state.range_x() / factor);
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t((state.range_x() / factor) * factor));

    delete [] buf;
}

static void BM_Memory_DoubleWordSet(benchmark::State &state)
{
    const int factor = 4;
    char *buf = new char[state.range_x()];

    while (state.KeepRunning())
    {
        DoubleWordSet(buf, 0xAB, state.range_x() / factor);
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t((state.range_x() / factor) * factor));

    delete [] buf;
}

static void BM_Memory_QuadWordSet(benchmark::State &state)
{
    const int factor = 8;
    char *buf = new char[state.range_x()];

    while (state.KeepRunning())
    {
        QuadWordSet(buf, 0xAB, state.range_x() / factor);
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t((state.range_x() / factor) * factor));

    delete [] buf;
}

static void BM_Memory_MemoryCompare(benchmark::State &state)
{
    // Two buffers with the same contents, which is the worst case for memcmp
    // performance (because every single byte will be visited).
    char *buf1 = new char[state.range_x()];
    char *buf2 = new char[state.range_x()];
    memset(buf1, 'a', state.range_x());
    memset(buf2, 'a', state.range_x());

    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(MemoryCompare(buf1, buf2, state.range_x()));

        // Poke the buffers to break the compiler's optimisations
        // (MemoryCompare is a pure function, so without an edit we literally
        // end up actually calling MemoryCompare once).
        ++(*buf1);
        ++(*buf2);
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range_x()));

    delete [] buf1;
    delete [] buf2;
}

// Test very large copies and sets for the base interfaces.
BENCHMARK(BM_Memory_MemoryCopy)->Range(8, 8<<24);
BENCHMARK(BM_Memory_OverlappedMemoryCopy)->Range(8, 8<<24);
BENCHMARK(BM_Memory_ForwardMemoryCopy)->Range(8, 8<<24);
BENCHMARK(BM_Memory_ByteSet)->Range(8, 8<<24);

// Smaller ranges for somewhat lesser benchmarks.
BENCHMARK(BM_Memory_ByteSetZero)->Range(8, 8<<16);
BENCHMARK(BM_Memory_WordSet)->Range(8, 8<<16);
BENCHMARK(BM_Memory_DoubleWordSet)->Range(8, 8<<16);
BENCHMARK(BM_Memory_QuadWordSet)->Range(8, 8<<16);
BENCHMARK(BM_Memory_MemoryCompare)->Range(8, 8<<16);
