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

#include <iostream>

#include <gtest/gtest.h>

#include <utilities/String.h>

// Output our String objects nicely (not as a list of bytes).
std::ostream &operator << (::std::ostream& os, const String &s)
{
    return os << "\"" << static_cast<const char *>(s) << "\" [" << static_cast<const void *>(s) << "]";
}

static const char *bigstring()
{
    static char buf[128] = {0};
    if (!buf[0])
    {
        memset(buf, 'a', 127);
    }
    return buf;
}

TEST(PedigreeString, Construction)
{
    String s("hello");
    EXPECT_EQ(s, "hello");

    String s2(bigstring());
    EXPECT_EQ(s2, bigstring());

    EXPECT_NE(s, s2);
}

TEST(PedigreeString, Length)
{
    String s("hello");
    EXPECT_EQ(s.length(), 5);
}

TEST(PedigreeString, Size)
{
    // 64-byte static strings
    String s("hello");
    EXPECT_EQ(s.size(), 64);

    // Dynamic strings are >64 bytes.
    String s2(bigstring());
    EXPECT_EQ(s2.size(), 128);
}

TEST(PedigreeString, Chomp)
{
    String s("hello ");
    s.chomp();
    EXPECT_EQ(s, "hello");
}

TEST(PedigreeString, Strip)
{
    String s(" hello ");
    s.strip();
    EXPECT_EQ(s, "hello");
}

TEST(PedigreeString, Rstrip)
{
    String s(" hello ");
    s.rstrip();
    EXPECT_EQ(s, " hello");
}

TEST(PedigreeString, Lstrip)
{
    String s(" hello ");
    s.lstrip();
    EXPECT_EQ(s, "hello ");
}

TEST(PedigreeString, UnneededLstrip)
{
    String s("hello ");
    s.lstrip();
    EXPECT_EQ(s, "hello ");
}

TEST(PedigreeString, UnneededRstrip)
{
    String s(" hello");
    s.rstrip();
    EXPECT_EQ(s, " hello");
}

TEST(PedigreeString, Split)
{
    String s("hello world");
    String right = s.split(5);
    EXPECT_EQ(s, "hello");
    EXPECT_EQ(right, " world");
}

TEST(PedigreeString, EmptyTokenise)
{
    String s("  a  ");
    List<String *> result = s.tokenise(' ');
    EXPECT_EQ(result.count(), 1);
    EXPECT_EQ(*(result.popFront()), "a");
}

TEST(PedigreeString, Tokenise)
{
    String s("hello world, this is a testcase that exercises tokenise");
    List<String *> result = s.tokenise(' ');
    EXPECT_EQ(*(result.popFront()), "hello");
    EXPECT_EQ(*(result.popFront()), "world,");
    EXPECT_EQ(*(result.popFront()), "this");
    EXPECT_EQ(*(result.popFront()), "is");
    EXPECT_EQ(*(result.popFront()), "a");
    EXPECT_EQ(*(result.popFront()), "testcase");
    EXPECT_EQ(*(result.popFront()), "that");
    EXPECT_EQ(*(result.popFront()), "exercises");
    EXPECT_EQ(*(result.popFront()), "tokenise");
}

TEST(PedigreeString, NextCharacter)
{
    String s("hello");
    EXPECT_EQ(s.nextCharacter(0), 1);
    EXPECT_EQ(s.nextCharacter(1), 2);
    EXPECT_EQ(s.nextCharacter(2), 3);
}

/// \todo(miselin): String::nextCharacter does not pass this today.
TEST(PedigreeString, DISABLED_NextCharacterUnicode)
{
    // 2-character UTF-8 in the middle of two single-byte characters.
    String s("h»b");
    EXPECT_EQ(s.nextCharacter(0), 1);
    EXPECT_EQ(s.nextCharacter(1), 3);
    EXPECT_EQ(s.nextCharacter(3), 4);
}

TEST(PedigreeString, Equality)
{
    // length differs
    EXPECT_NE(String("a"), String("ab"));
    // text differs
    EXPECT_NE(String("a"), String("b"));
    // big string differs in length
    EXPECT_NE(String("a"), String(bigstring()));
    // big vs big still matches
    EXPECT_EQ(String(bigstring()), String(bigstring()));
}

TEST(PedigreeString, AssignBig)
{
    String s;
    s.assign(bigstring());
    EXPECT_EQ(s, bigstring());
}

TEST(PedigreeString, AssignNothing)
{
    String s;
    s.assign("");
    EXPECT_EQ(s, "");
}

TEST(PedigreeString, AssignNull)
{
    String s;
    s.assign(0);
    EXPECT_EQ(s, "");
}

TEST(PedigreeString, AssignBigEmpty)
{
    String s;
    s.assign("a", 1024);
    EXPECT_EQ(s.size(), 1025);
    EXPECT_EQ(s, "a");
}

TEST(PedigreeString, AssignAnother)
{
    String s;
    String s2(bigstring());
    s.assign(s2);
    EXPECT_EQ(s, s2);
}

TEST(PedigreeString, ReduceReserve)
{
    // This should also not leak.
    String s;
    s.reserve(1024);
    EXPECT_EQ(s.size(), 1024);
    s.reserve(8);
    EXPECT_EQ(s.size(), 64);
}

TEST(PedigreeString, ReserveBoundary)
{
    String s;
    s.reserve(64);
    EXPECT_EQ(s.size(), 64);
}

TEST(PedigreeString, ReserveWithContent)
{
    String s("hello");
    s.reserve(64);
    EXPECT_EQ(s.size(), 64);
    EXPECT_EQ(s, "hello");
}

TEST(PedigreeString, SplitHuge)
{
    String s(bigstring());
    String right = s.split(32);

    EXPECT_EQ(s.length(), 32);
    EXPECT_EQ(right.length(), 128 - 32 - 1);
}

TEST(PedigreeString, Sprintf)
{
    String s;
    s.sprintf("Hello, %s! %d %d\n", "world", 42, 84);
    EXPECT_EQ(s, "Hello, world! 42 84\n");
}

TEST(PedigreeString, Free)
{
    String s("hello");
    s.free();
    EXPECT_EQ(s, "");
    EXPECT_EQ(s.length(), 0);
    EXPECT_EQ(s.size(), 0);
}

TEST(PedigreeString, FreeComparison)
{
    String s1("hello"), s2("hello");
    s1.free();
    EXPECT_NE(s1, s2);
    s2.free();
    EXPECT_EQ(s1, s2);
}

TEST(PedigreeString, FreeThenUse)
{
    String s("hello");
    s.free();
    s.assign("hello");
    EXPECT_EQ(s, "hello");
}

TEST(PedigreeString, FreeThenStrip)
{
    String s("hello");
    s.free();
    // Expecting no asan/valgrind/segfault errors, and no other funniness.
    s.strip();
    EXPECT_EQ(s, "");
    s.lstrip();
    EXPECT_EQ(s, "");
    s.rstrip();
    EXPECT_EQ(s, "");
}

TEST(PedigreeString, EndsWith)
{
    String s("hello");
    EXPECT_TRUE(s.endswith("ello"));
    EXPECT_TRUE(s.endswith(String("ello")));
}

TEST(PedigreeString, StartsWith)
{
    String s("hello");
    EXPECT_TRUE(s.startswith("hel"));
    EXPECT_TRUE(s.startswith(String("hel")));
}

TEST(PedigreeString, EndsWithIsEquality)
{
    String s("hello");
    EXPECT_TRUE(s.endswith("hello"));
    EXPECT_TRUE(s.endswith(String("hello")));
}

TEST(PedigreeString, StartsWithIsEquality)
{
    String s("hello");
    EXPECT_TRUE(s.startswith("hello"));
    EXPECT_TRUE(s.startswith(String("hello")));
}
