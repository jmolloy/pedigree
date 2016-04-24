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
#include <utilities/SharedPointer.h>
#include <compiler.h>

/** String class for ASCII strings
 *\todo provide documentation */
class String
{
    public:
        /** The default constructor does nothing */
        String();
        explicit String(const char *s);
        String(const char *s, size_t length);
        String(const String &x);
        ~String();

        String &operator = (const String &x);
        String &operator = (const char *s);
        operator const char *() const
        {
            if (m_Size == StaticSize)
                return m_Static;
            else if (m_Data == 0)
                return "";
            else
                return m_Data;
        }
        String &operator += (const String &x);
        String &operator += (const char *s);

        bool operator == (const String &s) const;
        bool operator == (const char *s) const;

        size_t length() const
        {
            return m_Length;
        }

        size_t size() const
        {
            return m_Size;
        }

        /** Given a character index, return the index of the next character, interpreting
            the string as UTF-8 encoded. */
        size_t nextCharacter(size_t c);

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

        List<SharedPointer<String>> tokenise(char token);

        /** Converts a UTF-32 character to its UTF-8 representation.
         *\param[in] utf32 Input UTF-32 character.
         *\param[out] utf8 Pointer to a buffer at least 4 bytes long.
         *\return The number of bytes in the UTF-8 string. */
        static size_t Utf32ToUtf8(uint32_t utf32, char *utf8)
        {
            *utf8 = utf32&0x7f;
            return 1;
        }

        void Format(const char *format, ...) FORMAT(printf, 2, 3);

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
        /** Internal doer for reserve() */
        void reserve(size_t size, bool zero);
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

        /** tokenise() pointer type. */
        typedef SharedPointer<String> tokenise_t;
};

/** @} */

#endif
