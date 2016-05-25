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

#include <utilities/utility.h>

TEST(PedigreeStringLibrary, StringLength)
{
    EXPECT_EQ(StringLength("hello"), 5);
    EXPECT_EQ(StringLength(""), 0);
}

TEST(PedigreeStringLibrary, BasicStrcpy)
{
    char buf[32] = {0};
    const char *source = "foobar";
    StringCopy(buf, source);
    EXPECT_STREQ(buf, source);
}

TEST(PedigreeStringLibrary, EmptyStrcpy)
{
    char buf[32] = {0};
    const char *source = "";
    StringCopy(buf, source);
    EXPECT_STREQ(buf, source);
}

TEST(PedigreeStringLibrary, EmbeddedNulStrcpy)
{
    char buf[32] = {0};
    const char *source = "abc\0def";
    StringCopy(buf, source);
    EXPECT_STREQ(buf, "abc");
}

TEST(PedigreeStringLibrary, BasicStrncpy)
{
    char buf[32] = {0};
    const char *source = "abcdef";
    StringCopyN(buf, source, 3);
    EXPECT_STREQ(buf, "abc");
}

TEST(PedigreeStringLibrary, EmptyStrncpy)
{
    char buf[32] = {0};
    const char *source = "abcdef";
    StringCopyN(buf, source, 0);
    EXPECT_STREQ(buf, "");
}

TEST(PedigreeStringLibrary, SmallFormat)
{
    char buf[32] = {0};
    StringFormat(buf, "Hello, %s!", "world");
    EXPECT_STREQ(buf, "Hello, world!");
}

TEST(PedigreeStringLibrary, CompareEmpty)
{
    EXPECT_EQ(StringCompare("", ""), 0);
}

TEST(PedigreeStringLibrary, CompareOneEmpty)
{
    EXPECT_EQ(StringCompare("abc", ""), 'a');
}

TEST(PedigreeStringLibrary, CompareOtherEmpty)
{
    // 'a' > '\0'
    EXPECT_EQ(StringCompare("", "abc"), -'a');
}

TEST(PedigreeStringLibrary, CompareSame)
{
    EXPECT_EQ(StringCompare("abc", "abc"), 0);
}

TEST(PedigreeStringLibrary, CompareLess)
{
    EXPECT_EQ(StringCompare("abc", "bcd"), 'a' - 'b');
}

TEST(PedigreeStringLibrary, CompareSome)
{
    EXPECT_EQ(StringCompareN("abcdef", "abc", 3), 0);
    EXPECT_EQ(StringCompareN("abcdef", "abc", 4), 'd');
    EXPECT_EQ(StringCompareN("abcdef", "abc", 1), 0);
    EXPECT_EQ(StringCompareN("abcdef", "abc", 0), 0);
}

TEST(PedigreeStringLibrary, CompareSomeOtherLonger)
{
    EXPECT_EQ(StringCompareN("abc", "abcdef", 3), 0);
    EXPECT_EQ(StringCompareN("abc", "abcdef", 4), -'d');
    EXPECT_EQ(StringCompareN("abc", "abcdef", 1), 0);
    EXPECT_EQ(StringCompareN("abc", "abcdef", 0), 0);
}

TEST(PedigreeStringLibrary, BasicStrcat)
{
    char buf[32] = {0};
    char *r = StringConcat(buf, "hello");
    EXPECT_STREQ(r, "hello");
}

TEST(PedigreeStringLibrary, EmptyStrcat)
{
    char buf[32] = {0};
    char *r = StringConcat(buf, "");
    EXPECT_STREQ(r, "");
}

TEST(PedigreeStringLibrary, BasicStrncat)
{
    char buf[32] = {0};
    char *r = StringConcatN(buf, "abcdef", 3);
    EXPECT_STREQ(r, "abc");
}

TEST(PedigreeStringLibrary, IsDigit)
{
    for (size_t i = 0; i < 10; ++i)
    {
        EXPECT_TRUE(isdigit('0' + i));
    }
}
