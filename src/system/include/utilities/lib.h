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

#ifndef KERNEL_UTILITIES_LIB_H
#define KERNEL_UTILITIES_LIB_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// String functions.
size_t StringLength(const char *buf);
char *StringCopy(char *dest, const char *src);
char *StringCopyN(char *dest, const char *src, size_t len);
int StringCompare(const char *p1, const char *p2);
int StringCompareN(const char *p1, const char *p2, size_t n);
char *StringConcat(char *dest, const char *src);
char *StringConcatN(char *dest, const char *src, size_t n);
const char *StringFind(const char *str, int target);
const char *StringReverseFind(const char *str, int target);
int VStringFormat(char *buf, const char *fmt, va_list arg);
int StringFormat(char *buf, const char *fmt, ...);
unsigned long StringToUnsignedLong(const char *nptr, char const **endptr, int base);

// Memory functions.
void *ByteSet(void *buf, int c, size_t len);
void *WordSet(void *buf, int c, size_t len);
void *DoubleWordSet(void *buf, unsigned int c, size_t len);
void *QuadWordSet(void *buf, unsigned long long c, size_t len);
void *ForwardMemoryCopy(void *s1, const void *s2, size_t n);
void *MemoryCopy(void *s1, const void *s2, size_t n);
int MemoryCompare(const void *p1, const void *p2, size_t len);

// Built-in PRNG.
void random_seed(uint64_t seed);
uint64_t random_next();

inline char toUpper(char c)
{
    if (c < 'a' || c > 'z')
        return c; // special chars
    c += ('A' - 'a');
    return c;
}

inline char toLower(char c)
{
    if (c < 'A' || c > 'Z')
        return c; // special chars
    c -= ('A' - 'a');
    return c;
}

#ifdef __cplusplus
}
#endif

#endif  // KERNEL_UTILITIES_LIB_H
