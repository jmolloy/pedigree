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

#include <cstdint>

#include <network-stack/TcpMisc.h>

TEST(PedigreeTcpBuffer, InitialSettings)
{
    TcpBuffer buffer;

    EXPECT_EQ(buffer.getDataSize(), 0);
    EXPECT_EQ(buffer.getSize(), 32768);
    EXPECT_EQ(buffer.getRemainingSize(), 32768);
}

TEST(PedigreeTcpBuffer, ReadEmpty)
{
    TcpBuffer buffer;

    char buf[16];
    memset(buf, 0, 16);

    size_t r = buffer.read(reinterpret_cast<uintptr_t>(buf), 16);

    EXPECT_EQ(r, 0);
}

TEST(PedigreeTcpBuffer, TooMany)
{
    TcpBuffer buffer;
    buffer.setSize(8);

    char buf[16];
    memset(buf, 0, 16);

    EXPECT_EQ(buffer.write(reinterpret_cast<uintptr_t>(buf), 16), 8);
    EXPECT_EQ(buffer.read(reinterpret_cast<uintptr_t>(buf), 16), 8);
}

TEST(PedigreeTcpBuffer, ReadTooMany)
{
    TcpBuffer buffer;
    buffer.setSize(8);

    char buf[16];
    memset(buf, 0, 16);

    EXPECT_EQ(buffer.write(reinterpret_cast<uintptr_t>(buf), 6), 6);
    EXPECT_EQ(buffer.read(reinterpret_cast<uintptr_t>(buf), 8), 6);
}

TEST(PedigreeTcpBuffer, Overflow)
{
    TcpBuffer buffer;
    buffer.setSize(8);

    char buf[16];
    memset(buf, 0xAB, 16);

    // Overflow truncates.
    EXPECT_EQ(buffer.write(reinterpret_cast<uintptr_t>(buf), 16), 8);
    EXPECT_EQ(buffer.getDataSize(), 8);
    EXPECT_EQ(buffer.getRemainingSize(), 0);
}

TEST(PedigreeTcpBuffer, OverlapReader)
{
    TcpBuffer buffer;
    buffer.setSize(8);

    char buf[16], buf2[16];
    memset(buf, 0xAB, 16);

    // Write 6 bytes, read 2. Reader is 2 bytes into the buffer. This means we
    // have 4 bytes available, so we'll overlap, which cuts the number of bytes
    // the writer actually writes (to avoid the overlap).
    EXPECT_EQ(buffer.write(reinterpret_cast<uintptr_t>(buf), 6), 6);
    EXPECT_EQ(buffer.read(reinterpret_cast<uintptr_t>(buf2), 2), 2);
    EXPECT_EQ(buffer.write(reinterpret_cast<uintptr_t>(buf), 4), 4);
}

TEST(PedigreeTcpBuffer, Overlap)
{
    TcpBuffer buffer;
    buffer.setSize(8);

    char buf[16], buf2[16];
    memset(buf, 0xAB, 16);

    EXPECT_EQ(buffer.write(reinterpret_cast<uintptr_t>(buf), 6), 6);
    EXPECT_EQ(buffer.read(reinterpret_cast<uintptr_t>(buf2), 6), 6);
    EXPECT_TRUE(memcmp(buf, buf2, 6) == 0);

    // Rolls over - two bytes at the end and then two bytes at the start.
    EXPECT_EQ(buffer.write(reinterpret_cast<uintptr_t>(buf), 4), 4);
    EXPECT_EQ(buffer.read(reinterpret_cast<uintptr_t>(buf2), 4), 4);

    // Verify the reader can catch up.
    EXPECT_TRUE(memcmp(buf, buf2, 4) == 0);
}

TEST(PedigreeTcpBuffer, FillBuffer)
{
    TcpBuffer buffer;

    char buf[16];
    memset(buf, 0xAB, 16);

    // Limited iteration, but should end much sooner.
    for (size_t i = 0; i < 32768; ++i)
    {
        size_t r = buffer.write(reinterpret_cast<uintptr_t>(buf), 16);
        if (r < 16)
            break;
    }

    // Can't write to a full buffer.
    EXPECT_EQ(buffer.write(reinterpret_cast<uintptr_t>(buf), 16), 0);

    EXPECT_EQ(buffer.getRemainingSize(), 0);
}

TEST(PedigreeTcpBuffer, FillAndRead)
{
    TcpBuffer buffer;

    char buf[32768], out[32768];
    memset(buf, 0xAB, 32768);
    memset(out, 0, 32768);

    size_t r = buffer.write(reinterpret_cast<uintptr_t>(buf), 32768);
    EXPECT_EQ(r, 32768);

    r = buffer.read(reinterpret_cast<uintptr_t>(out), 32768);
    EXPECT_EQ(r, 32768);

    EXPECT_TRUE(memcmp(buf, out, 32768) == 0);
}

TEST(PedigreeTcpBuffer, Chase)
{
    TcpBuffer buffer;

    const int n = 0x10000;
    const int readThreshold = 0x500;

    size_t *numbers = new size_t[n];
    size_t offset = 0;

    for (size_t i = 0; i < n; ++i)
    {
        buffer.write(reinterpret_cast<uintptr_t>(&i), sizeof(i));

        if (i && (i % readThreshold == 0))
        {
            // Read entries into our buffer.
            buffer.read(reinterpret_cast<uintptr_t>(numbers) + offset,
                        readThreshold * sizeof(size_t));
            offset += readThreshold * sizeof(size_t);
        }
    }

    buffer.read(reinterpret_cast<uintptr_t>(numbers) + offset, buffer.getDataSize());

    for (size_t i = 0; i < n; ++i)
    {
        // Should match, as we chased the buffer.
        EXPECT_EQ(numbers[i], i);
        if (numbers[i] != i)
            break;
    }

    delete [] numbers;
}
