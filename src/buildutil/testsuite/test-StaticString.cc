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

#include <utilities/StaticString.h>

// Test fixture for typed tests.
template <typename T>
class PedigreeStaticStringSignedTypes : public ::testing::Test {};
template <typename T>
class PedigreeStaticStringUnsignedTypes : public ::testing::Test {};

typedef ::testing::Types<short, int, long, long long> SignedIntegerTypes;
typedef ::testing::Types<unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long> UnsignedIntegerTypes;
TYPED_TEST_CASE(PedigreeStaticStringSignedTypes, SignedIntegerTypes);
TYPED_TEST_CASE(PedigreeStaticStringUnsignedTypes, UnsignedIntegerTypes);

TEST(PedigreeStaticString, Construction)
{
    StaticString<64> s;
    EXPECT_STREQ(s, "");
}

TEST(PedigreeStaticString, ConstructionFromString)
{
    StaticString<64> s("hello");
    EXPECT_STREQ(s, "hello");
    EXPECT_EQ(s.length(), 5);
}

TEST(PedigreeStaticString, ConstructionFromTooLongString)
{
    StaticString<3> s("hello");
    EXPECT_STREQ(s, "he");
    EXPECT_EQ(s.length(), 2);
}

TEST(PedigreeStaticString, ConstructionFromStaticString)
{
    StaticString<64> other("hello");
    StaticString<64> s(other);
    EXPECT_STREQ(s, "hello");
    EXPECT_EQ(s.length(), 5);
}

TEST(PedigreeStaticString, ConstructionFromTooLongStaticString)
{
    StaticString<64> other("hello");
    StaticString<3> s(other);
    EXPECT_STREQ(s, "he");
    EXPECT_EQ(s.length(), 2);
}

TEST(PedigreeStaticString, AppendOperator)
{
    StaticString<64> s("hello");
    s += " world";
    EXPECT_STREQ(s, "hello world");
}

TEST(PedigreeStaticString, Clear)
{
    StaticString<64> s("hello");
    s.clear();
    EXPECT_STREQ(s, "");
}

TEST(PedigreeStaticString, AssignOperator)
{
    StaticString<64> s("hello");
    s = "goodbye";
    EXPECT_STREQ(s, "goodbye");
}

TEST(PedigreeStaticString, AssignOperatorTooLong)
{
    StaticString<5> s("hello");
    s = "goodbye";
    EXPECT_STREQ(s, "good");
}

TEST(PedigreeStaticString, Equality)
{
    StaticString<64> s("hello");
    EXPECT_EQ(s, "hello");
}

TEST(PedigreeStaticString, EqualityToOtherStaticString)
{
    StaticString<64> s1("hello");
    StaticString<64> s2("hello");
    EXPECT_EQ(s1, s2);
}

TEST(PedigreeStaticString, First)
{
    StaticString<64> s("hello");
    EXPECT_EQ(s.first('e'), 1);
    EXPECT_EQ(s.first('z'), -1);
}

TEST(PedigreeStaticString, Last)
{
    StaticString<64> s("hello");
    EXPECT_EQ(s.last('l'), 3);
    EXPECT_EQ(s.last('z'), -1);
}

TEST(PedigreeStaticString, StripLast)
{
    StaticString<64> s("hello");
    s.stripLast();
    EXPECT_STREQ(s, "hell");
}

TEST(PedigreeStaticString, Contains)
{
    StaticString<64> s("hello");
    EXPECT_TRUE(s.contains("ell"));
    EXPECT_TRUE(s.contains("hello"));
    EXPECT_FALSE(s.contains("world"));
    EXPECT_FALSE(s.contains("goo"));
}

TEST(PedigreeStaticString, ContainsStaticString)
{
    StaticString<64> other("ell");
    StaticString<64> s("hello");
    EXPECT_TRUE(s.contains(other));
    other = "world";
    EXPECT_FALSE(s.contains(other));
    other = "goo";
    EXPECT_FALSE(s.contains(other));
}

TEST(PedigreeStaticString, IntValue)
{
    StaticString<64> s("1234");
    EXPECT_EQ(s.intValue(10), 1234);
}

TEST(PedigreeStaticString, BadIntValue)
{
    StaticString<64> s("foo");
    EXPECT_EQ(s.intValue(10), -1);
}

TEST(PedigreeStaticString, IntPtrValue)
{
    StaticString<64> s("1234");
    EXPECT_EQ(s.uintptrValue(10), 1234);
}

TEST(PedigreeStaticString, BadIntPtrValue)
{
    StaticString<64> s("foo");
    EXPECT_EQ(s.uintptrValue(10), ~0UL);
}

TEST(PedigreeStaticString, Truncate)
{
    StaticString<64> s("hello");
    s.truncate(2);
    EXPECT_STREQ(s, "he");
}

TEST(PedigreeStaticString, TruncateNoOp)
{
    StaticString<64> s("hello");
    s.truncate(64);
    EXPECT_STREQ(s, "hello");
}

TEST(PedigreeStaticString, Left)
{
    StaticString<64> s("hello");
    StaticString<64> s2 = s.left(2);
    EXPECT_STREQ(s, "hello");
    EXPECT_STREQ(s2, "he");
}

TEST(PedigreeStaticString, Right)
{
    StaticString<64> s("hello");
    StaticString<64> s2 = s.right(2);
    EXPECT_STREQ(s, "hello");
    EXPECT_STREQ(s2, "lo");
}

TEST(PedigreeStaticString, StripFirst)
{
    StaticString<64> s("hello");
    s.stripFirst(3);
    EXPECT_STREQ(s, "lo");
    s.stripFirst(5);
    EXPECT_STREQ(s, "");
}

TEST(PedigreeStaticString, StreamInsertion)
{
    StaticString<64> s("hello");
    s << " world" << "!";
    EXPECT_STREQ(s, "hello world!");
}

TEST(PedigreeStaticString, AppendChar)
{
    StaticString<64> s("hello");
    s.append('!');
    EXPECT_STREQ(s, "hello!");
}

TEST(PedigreeStaticString, AppendCharTooLong)
{
    StaticString<6> s("hello");
    s.append('!');
    EXPECT_STREQ(s, "hello");
}

TEST(PedigreeStaticString, AppendStaticString)
{
    StaticString<64> s("hello");
    StaticString<64> other("!");
    s.append(other);
    EXPECT_STREQ(s, "hello!");
}

TEST(PedigreeStaticString, AppendStaticStringTooLong)
{
    StaticString<6> s("hello");
    StaticString<64> other("!");
    s.append(other);
    EXPECT_STREQ(s, "hello");
}

TYPED_TEST(PedigreeStaticStringSignedTypes, AppendSignedIntegers)
{
    StaticString<64> s("hello");
    s.append(static_cast<TypeParam>(50));
    EXPECT_STREQ(s, "hello50");
    s.append(static_cast<TypeParam>(-5));
    EXPECT_STREQ(s, "hello50-5");
}

TYPED_TEST(PedigreeStaticStringUnsignedTypes, AppendUnsignedIntegers)
{
    StaticString<64> s("hello");
    s.append(static_cast<TypeParam>(50));
    EXPECT_STREQ(s, "hello50");
}

TEST(PedigreeStaticString, PaddedIntegerAppend)
{
    StaticString<64> s("hello ");
    s.append(5, 10, 4, 'x');
    EXPECT_STREQ(s, "hello xxx5");
    s.append(15, 16, 4, 'x');
    EXPECT_STREQ(s, "hello xxx5xxxf");
}

TEST(PedigreeStaticString, PaddedStringAppend)
{
    StaticString<64> s("hello ");
    s.append("world", 6, '!');
    EXPECT_STREQ(s, "hello !world");
}

TEST(PedigreeStaticString, PaddedStaticStringAppend)
{
    StaticString<64> s("hello ");
    StaticString<64> other("world");
    s.append(other, 6, '!');
    EXPECT_STREQ(s, "hello !world");
}

TEST(PedigreeStaticString, Pad)
{
    StaticString<64> s("hello");
    s.pad(10, 'x');
    EXPECT_STREQ(s, "helloxxxxx");
}
