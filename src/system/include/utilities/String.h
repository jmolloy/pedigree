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

#ifndef KERNEL_UTILITIES_STRING_H
#define KERNEL_UTILITIES_STRING_H

/** @addtogroup kernelutilities
 * @{ */

#include <processor/types.h>
#include <utilities/List.h>
#include <utilities/utility.h>

/** String class for ASCII strings
 *\todo provide documentation */
class String
{
    public:
        /** The default constructor does nothing */
        inline String();
        inline explicit String(const char *s);
        inline String(const String &x);
        inline ~String();

        inline String &operator = (const String &x);
        inline String &operator = (const char *s);
        inline operator const char *() const;
        inline String &operator += (const String &x);
        inline String &operator += (const char *s);

        inline bool operator == (const String &s) const;
        inline bool operator == (const char *s) const;

        inline size_t length() const;
        inline size_t size() const;

        /** Given a character index, return the index of the next character, interpreting
            the string as UTF-8 encoded. */
        inline size_t nextCharacter(size_t c);

        /** Removes the last character from the string. */
        void chomp();

        /** Removes the whitespace from the both ends of the string. */
        void strip();

        /** Removes the whitespace from the start of the string. */
        void lstrip();

        /** Removes the whitespace from the end of the string. */
        void rstrip();

        /** Splits the string at the given offset - the front portion will be kept in this string,
         *  the back portion (including the character at 'offset' will be returned in a new string. */
        String split(size_t offset);

        List<String*> tokenise(char token);

        /** Converts a UTF-32 character to its UTF-8 representation.
         *\param[in] utf32 Input UTF-32 character.
         *\param[out] utf8 Pointer to a buffer at least 4 bytes long.
         *\return The number of bytes in the UTF-8 string. */
        static size_t Utf32ToUtf8(uint32_t utf32, char *utf8)
        {
            *utf8 = utf32&0x7f;
            return 1;
        }

        void sprintf(const char *format, ...);

        void assign(const String &x);
        void assign(const char *s, size_t len = 0);
        void reserve(size_t size);
        void free();

        /** Does this string end with the given string? */
        bool endswith(const String &s) const;
        bool endswith(const char *s) const;

        /** Does this string start with the given string? */
        bool startswith(const String &s) const;
        bool startswith(const char *s) const;

    private:
        /** Size of static string storage (over this threshold, the heap is used) */
        static const size_t StaticSize = 64;
        /** Pointer to the zero-terminated ASCII string */
        char *m_Data;
        /** The string's length */
        size_t m_Length;
        /** The size of the reserved space for the string */
        size_t m_Size;
        /** Static string storage (avoid heap overhead for small strings) */
        char m_Static[StaticSize];
        /** Is the given character whitespace? (for *strip()) */
        bool iswhitespace(const char c) const;
};

/** @} */

//
// Part of the implementation
//
String::String()
    : m_Data(0), m_Length(0), m_Size(StaticSize), m_Static()
{
}
String::String(const char *s)
    : m_Data(0), m_Length(0), m_Size(StaticSize), m_Static()
{
    assign(s);
}
String::String(const String &x)
    : m_Data(0), m_Length(0), m_Size(StaticSize), m_Static()
{
    assign(x);
}
String::~String()
{
    free();
}

String &String::operator = (const String &x)
{
    assign(x);
    return *this;
}
String &String::operator = (const char *s)
{
    assign(s);
    return *this;
}
String::operator const char *() const
{
    if (m_Size == StaticSize)
        return m_Static;
    else if (m_Data == 0)
        return "";
    else
        return m_Data;
}

String &String::operator += (const String &x)
{
    size_t newLength = x.length() + m_Length;

    char *dst = m_Static;

    // Do we need to transfer static into dynamic for this?
    if (newLength >= StaticSize)
    {
        reserve(newLength + 1);
        if (m_Length < StaticSize)
            memcpy(m_Data, m_Static, m_Length);
        dst = m_Data;
    }

    const char *src = x.m_Static;
    if (x.length() > StaticSize)
        src = x.m_Data;

    // Copy!
    memcpy(&dst[m_Length], src, x.length() + 1);
    m_Length += x.length();
    return *this;
}
String &String::operator += (const char *s)
{
    size_t slen = strlen(s);
    size_t newLength = slen + m_Length;
    if (newLength < StaticSize)
    {
        // By the nature of the two lengths combined being below the static
        // size, we can be assured that we can use the static buffer in
        // both strings.
        memcpy(&m_Static[m_Length], s, slen + 1);
    }
    else
    {
        reserve(slen + m_Length + 1);
        if (m_Length < StaticSize)
            memcpy(m_Data, m_Static, m_Length);
        memcpy(&m_Data[m_Length], s, slen + 1);
    }

    m_Length += slen;
    return *this;
}

bool String::operator == (const String &s) const
{
    if (m_Length != s.m_Length)
        return false;
    else if ((m_Length < StaticSize && s.m_Length >= StaticSize) ||
            (m_Length >= StaticSize && s.m_Length < StaticSize))
        return false;
    else if(m_Length < StaticSize)
        return !strcmp(m_Static, s.m_Static);
    if (m_Data == 0 && s.m_Data == 0)
        return true;
    else if (m_Data == 0 || s.m_Data == 0)
        return false;
    else
        return !strcmp(m_Data, s.m_Data);
}

bool String::operator == (const char *s) const
{
    const char *buf = m_Data;
    if (m_Length < StaticSize)
        buf = m_Static;
    if (((buf == 0) || (m_Length == 0)) && s == 0)
        return true;
    else if (buf == 0 || s == 0)
    {
        if (buf == 0)
        {
            size_t otherLength = strlen(s);
            return otherLength == 0;
        }
        else  // s == 0
        {
            return m_Length == 0;
        }
    }
    else
        return !strcmp(buf, s);
}

size_t String::length() const
{
    return m_Length;
}
size_t String::size() const
{
    return m_Size;
}

size_t String::nextCharacter(size_t c)
{
    // TODO handle multibyte chars.
    return c+1;
}

#endif
