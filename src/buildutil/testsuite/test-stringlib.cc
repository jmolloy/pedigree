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

/// \todo this may unintentionally test the host's libc instead of Pedigree.

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
    EXPECT_EQ(StringCompare("abc", ""), 1);
}

TEST(PedigreeStringLibrary, CompareOtherEmpty)
{
    // 'a' > '\0'
    EXPECT_EQ(StringCompare("", "abc"), 1);
}

TEST(PedigreeStringLibrary, CompareSame)
{
    EXPECT_EQ(StringCompare("abc", "abc"), 0);
}

TEST(PedigreeStringLibrary, CompareLess)
{
    /// \todo make this actually check against the intended result (1, -1)
    EXPECT_NE(StringCompare("abc", "bcd"), 0);
}

/// \todo add strncmp tests here

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
