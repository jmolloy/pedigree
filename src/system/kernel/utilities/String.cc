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

void String::assign(const String &x)
{
    m_Length = x.length();
    if (m_Length < StaticSize)
    {
        memcpy(m_Static, x.m_Static, m_Length + 1);
        delete [] m_Data;
        m_Data = 0;
        m_Size = StaticSize;
    }
    else
    {
        reserve(m_Length + 1);
        if (m_Length && x.m_Data)
            memcpy(m_Data, x.m_Data, m_Length + 1);
        else
            m_Data[0] = '\0';
    }

#ifdef ADDITIONAL_CHECKS
    assert(*this == x);
#endif
}
void String::assign(const char *s, size_t len)
{
    if (!s || !*s)
        m_Length = 0;
    else if (len)
        m_Length = len;
    else
        m_Length = strlen(s);

    if (!m_Length)
    {
        memset(m_Static, 0, StaticSize);
        delete [] m_Data;
        m_Data = 0;
        m_Size = StaticSize;
    }
    else if (m_Length < StaticSize)
    {
        memcpy(m_Static, s, m_Length);
        delete [] m_Data;
        m_Data = 0;
        m_Size = StaticSize;
        m_Static[m_Length] = '\0';
    }
    else
    {
        reserve(m_Length + 1);
        if (m_Length && s)
        {
            memcpy(m_Data, s, m_Length);
            m_Data[m_Length] = '\0';
        }
        else
            m_Data[0] = '\0';
    }

#ifdef ADDITIONAL_CHECKS
    if (!len)
        assert(*this == s);
#endif
}
void String::reserve(size_t size)
{
    // Don't reserve if we're a static string.
    if (size <= StaticSize)
    {
        if (m_Size > StaticSize)
        {
            m_Size = StaticSize;
            memcpy(m_Static, m_Data, size);
            delete [] m_Data;
            m_Data = 0;
        }

        return;
    }
    if ((size == m_Size) && (m_Size == StaticSize))
        ++size; // Switching from static to dynamic.

    if (size > m_Size)
    {
        char *tmp = m_Data;
        m_Data = new char [size];
        memset(m_Data, 0, size);
        if (tmp != 0)
        {
            memcpy(m_Data, tmp, m_Size > size ? size : m_Size);
            delete [] tmp;
        }
        m_Size = size;
    }
}
void String::free()
{
    delete [] m_Data;
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
        memcpy(m_Static, buf, m_Length);
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

    // Sanity check...
    if(!buf)
        return;
    
    if(!iswhitespace(buf[0]))
        return;

    size_t n = 0;
    while(n < m_Length && iswhitespace(buf[n]))
        n++;

    // Move the data to cover up the whitespace and avoid reallocating m_Data
    m_Length -= n;
    memmove(buf, (buf + n), m_Length);
    buf[m_Length] = 0;

    // Did we suddenly drop below the static size?
    if ((buf == m_Data) && (m_Length < StaticSize))
    {
        memcpy(m_Static, m_Data, m_Length + 1);
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

    // Sanity check...
    if(!buf)
        return;

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
        memcpy(m_Static, m_Data, m_Length + 1);
        m_Size = StaticSize;
        delete [] m_Data;
        m_Data = 0;
    }
}

List<String*> String::tokenise(char token)
{
    String copy = *this;
    List<String*> list;

    size_t idx = 0;
    while (idx < copy.m_Length)
    {
        if (copy[idx] == token)
        {
            String tmp = copy.split(idx+1);

            String *pStr = new String(copy);
            copy = tmp;

            // pStr will include token, so remove the last character from it.
            pStr->chomp();

            if (pStr->length() == 0)
                delete pStr;
            else
                list.pushBack(pStr);
            idx = 0;
        }
        else
            idx = copy.nextCharacter(idx);
    }

    if (copy.length() > 0)
        list.pushBack(new String(copy));

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
        memcpy(m_Static, m_Data, m_Length + 1);
        m_Size = StaticSize;
        delete [] m_Data;
        m_Data = 0;
    }
}

void String::sprintf(const char *fmt, ...)
{
    reserve(256);
    va_list vl;
    va_start(vl, fmt);
    m_Length = vsprintf(m_Data, fmt, vl);
    va_end(vl);

    if (m_Length < StaticSize)
    {
        memcpy(m_Static, m_Data, m_Length + 1);
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
    return !memcmp(mybuf, otherbuf, s.length());
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
    return !memcmp(mybuf, otherbuf, s.length());
}

bool String::startswith(const char *s) const
{
    return startswith(String(s));
}

bool String::iswhitespace(const char c) const
{
    return (c <= ' ' || c == '\x7f');
}
