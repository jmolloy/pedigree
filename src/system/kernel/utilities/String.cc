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

#include <utilities/utility.h>
#include <utilities/String.h>

String::String()
    : m_Data(0), m_Length(0), m_Size(StaticSize), m_Static()
{
}

String::String(const char *s)
    : m_Data(0), m_Length(0), m_Size(StaticSize), m_Static()
{
    assign(s);
}

String::String(const char *s, size_t length)
    : m_Data(0), m_Length(0), m_Size(StaticSize), m_Static()
{
    assign(s, length);
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
            MemoryCopy(m_Data, m_Static, m_Length);
        dst = m_Data;
    }

    const char *src = x.m_Static;
    if (x.length() > StaticSize)
        src = x.m_Data;

    // Copy!
    MemoryCopy(&dst[m_Length], src, x.length() + 1);
    m_Length += x.length();
    return *this;
}

String &String::operator += (const char *s)
{
    size_t slen = StringLength(s);
    size_t newLength = slen + m_Length;
    if (newLength < StaticSize)
    {
        // By the nature of the two lengths combined being below the static
        // size, we can be assured that we can use the static buffer in
        // both strings.
        MemoryCopy(&m_Static[m_Length], s, slen + 1);
    }
    else
    {
        reserve(slen + m_Length + 1);
        if (m_Length < StaticSize)
            MemoryCopy(m_Data, m_Static, m_Length);
        MemoryCopy(&m_Data[m_Length], s, slen + 1);
    }

    m_Length += slen;
    return *this;
}

bool String::operator == (const String &s) const
{
    if (m_Length != s.m_Length)
        return false;
    else if(m_Length < StaticSize)
        return !StringCompare(m_Static, s.m_Static);

    // Neither of these can be null because of the above conditions.
    return !StringCompare(m_Data, s.m_Data);
}

bool String::operator == (const char *s) const
{
    const char *buf = m_Data;
    if (m_Length < StaticSize)
        buf = m_Static;

    if ((!m_Length) && s == 0)
        return true;
    else if (s == 0)
        // m_Length > 0 but other buffer is null.
        return false;
    else
        return !StringCompare(buf, s);
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

void String::assign(const String &x)
{
    m_Length = x.length();
    if (m_Length < StaticSize)
    {
        MemoryCopy(m_Static, x.m_Static, m_Length + 1);
        delete [] m_Data;
        m_Data = 0;
        m_Size = StaticSize;
    }
    else
    {
        // Length is bigger than a static buffer, no need to check for empty
        // buffer.
        reserve(m_Length + 1, false);
        MemoryCopy(m_Data, x.m_Data, m_Length + 1);
    }

#ifdef ADDITIONAL_CHECKS
    assert(*this == x);
#endif
}
void String::assign(const char *s, size_t len)
{
    size_t copyLength = 0;
    if (!s || !*s)
        m_Length = 0;
    else if (len)
    {
        m_Length = len;
        copyLength = len;
    }
    else
    {
        m_Length = StringLength(s);
        copyLength = m_Length;
    }

    if (!m_Length)
    {
        ByteSet(m_Static, 0, StaticSize);
        delete [] m_Data;
        m_Data = 0;
        m_Size = StaticSize;
    }
    else if (m_Length < StaticSize)
    {
        MemoryCopy(m_Static, s, copyLength);
        delete [] m_Data;
        m_Data = 0;
        m_Size = StaticSize;
        m_Static[copyLength] = '\0';
    }
    else
    {
        reserve(m_Length + 1, false);
        MemoryCopy(m_Data, s, copyLength);
        m_Data[copyLength] = '\0';
    }

#ifdef ADDITIONAL_CHECKS
    if (!len)
        assert(*this == s);
#endif
}

void String::reserve(size_t size)
{
    reserve(size, true);
}

void String::reserve(size_t size, bool zero)
{
    // Don't reserve if we're a static string.
    if (size <= StaticSize)
    {
        if (m_Size > StaticSize)
        {
            m_Size = StaticSize;
            MemoryCopy(m_Static, m_Data, size);
            delete [] m_Data;
            m_Data = 0;
        }

        return;
    }
    else if (size > m_Size)
    {
        char *tmp = m_Data;
        m_Data = new char [size];
        if (tmp)
        {
            MemoryCopy(m_Data, tmp, m_Size > size ? size : m_Size);
            delete [] tmp;
        }
        else if (zero)
        {
            ByteSet(m_Data, 0, size);
        }
        m_Size = size;
    }
}
void String::free()
{
    delete [] m_Data;
    ByteSet(m_Static, 0, StaticSize);
    m_Data = 0;
    m_Length = 0;
    m_Size = 0;
}

String String::split(size_t offset)
{
    char *buf = m_Data;
    if (m_Length < StaticSize)
        buf = m_Static;

    String s(&buf[offset]);
    m_Length = offset;

    // Handle the case where the split causes our string to suddenly be shorter
    // than the static size.
    if((m_Length < StaticSize) && (buf == m_Data))
    {
        MemoryCopy(m_Static, buf, m_Length);
        buf = m_Static;

        delete [] m_Data;
        m_Data = 0;
        m_Size = StaticSize;
    }

    buf[m_Length] = 0;

    return s;
}

void String::strip()
{
    lstrip();
    rstrip();
}

void String::lstrip()
{
    char *buf = m_Data;
    if (m_Length < StaticSize)
        buf = m_Static;
    
    if(!iswhitespace(buf[0]))
        return;

    size_t n = 0;
    while(n < m_Length && iswhitespace(buf[n]))
        n++;

    // Move the data to cover up the whitespace and avoid reallocating m_Data
    m_Length -= n;
    MemoryCopy(buf, (buf + n), m_Length);
    buf[m_Length] = 0;

    // Did we suddenly drop below the static size?
    if ((buf == m_Data) && (m_Length < StaticSize))
    {
        MemoryCopy(m_Static, m_Data, m_Length + 1);
        m_Size = StaticSize;
        delete [] m_Data;
        m_Data = 0;
    }
}

void String::rstrip()
{
    char *buf = m_Data;
    if (m_Length < StaticSize)
        buf = m_Static;

    if(!iswhitespace(buf[m_Length - 1]))
        return;

    size_t n = m_Length;
    while(n > 0 && iswhitespace(buf[n - 1]))
        n--;

    // m_Size is still valid - it's the size of the buffer. m_Length is now
    // updated to contain the proper length of the string, but the buffer is
    // not reallocated.
    m_Length = n;
    buf[m_Length] = 0;

    // Did we suddenly drop below the static size?
    if ((buf == m_Data) && (m_Length < StaticSize))
    {
        MemoryCopy(m_Static, m_Data, m_Length + 1);
        m_Size = StaticSize;
        delete [] m_Data;
        m_Data = 0;
    }
}

List<SharedPointer<String>> String::tokenise(char token)
{
    List<tokenise_t> list;
    const char *orig_buffer = static_cast<const char *>(*this);
    const char *buffer = orig_buffer;

    const char *pos = nullptr;
    while (*buffer)
    {
        pos = StringFind(buffer, token);
        if (!pos)
        {
            break;
        }

        if (pos == buffer)
        {
            ++buffer;
            continue;
        }

        tokenise_t pStr = tokenise_t(new String(buffer, pos - buffer));
        if (pStr->length())
            list.pushBack(pStr);

        buffer = pos + 1;
    }

    if (!pos)
    {
        tokenise_t pStr = tokenise_t(new String(buffer, m_Length - (buffer - orig_buffer)));
        if (pStr->length())
            list.pushBack(pStr);
    }

    return list;
}

void String::chomp()
{
    char *buf = m_Data;
    if (m_Length < StaticSize)
        buf = m_Static;

    m_Length --;
    buf[m_Length] = '\0';

    // Did we suddenly drop below the static size?
    if ((buf == m_Data) && (m_Length < StaticSize))
    {
        MemoryCopy(m_Static, m_Data, m_Length + 1);
        m_Size = StaticSize;
        delete [] m_Data;
        m_Data = 0;
    }
}

void String::Format(const char *fmt, ...)
{
    reserve(256);
    va_list vl;
    va_start(vl, fmt);
    m_Length = VStringFormat(m_Data, fmt, vl);
    va_end(vl);

    if (m_Length < StaticSize)
    {
        MemoryCopy(m_Static, m_Data, m_Length + 1);
        m_Size = StaticSize;
        delete [] m_Data;
        m_Data = 0;
    }
}

bool String::endswith(const String &s) const
{
    // Not a suffix check.
    if(m_Length == s.length())
        return *this == s;

    // Suffix exceeds our length.
    if(m_Length < s.length())
        return false;

    const char *mybuf = m_Data;
    if(m_Length < StaticSize)
        mybuf = m_Static;
    mybuf += m_Length - s.length();

    const char *otherbuf = s.m_Data;
    if(s.length() < StaticSize)
        otherbuf = s.m_Static;

    // Do the check.
    return !MemoryCompare(mybuf, otherbuf, s.length());
}

bool String::endswith(const char *s) const
{
    return endswith(String(s));
}

bool String::startswith(const String &s) const
{
    // Not a prefix check.
    if(m_Length == s.length())
        return *this == s;

    // Prefix exceeds our length.
    if(m_Length < s.length())
        return false;

    const char *mybuf = m_Data;
    if(m_Length < StaticSize)
        mybuf = m_Static;

    const char *otherbuf = s.m_Data;
    if(s.length() < StaticSize)
        otherbuf = s.m_Static;

    // Do the check.
    return !MemoryCompare(mybuf, otherbuf, s.length());
}

bool String::startswith(const char *s) const
{
    return startswith(String(s));
}

bool String::iswhitespace(const char c) const
{
    return (c <= ' ' || c == '\x7f');
}
