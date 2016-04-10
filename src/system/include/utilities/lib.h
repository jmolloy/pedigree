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

size_t strlen(const char *buf);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t len);
void *memset(void *buf, int c, size_t len);
void *dmemset(void *buf, unsigned int c, size_t len);
void *qmemset(void *buf, unsigned long long c, size_t len);
void *memcpy(void *s1, const void *s2, size_t n);
void *memmove(void *s1, const void *s2, size_t n);
int memcmp(const void *p1, const void *p2, size_t len);

int strcmp(const char *p1, const char *p2);
int strncmp(const char *p1, const char *p2, size_t n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);

void random_seed(uint64_t seed);
uint64_t random_next();

char *strchr(const char *str, int target);
char *strrchr(const char *str, int target);

#ifdef UTILITY_LINUX
#include <stdio.h>
#else
// When using parts of the kernel for tools to run on build systems, we can run
// into conflicts with some of our functions. So, we only define them if we are
// not building for build systems.
void *wmemset(void *buf, int c, size_t len);
int vsprintf(char *buf, const char *fmt, va_list arg);
int sprintf(char *buf, const char *fmt, ...);
#endif

/**
 * Custom API that differs from the standard strtoul and therefore conflicts.
 *
 * The main difference is the const-ness of the endptr parameter. The outer
 * pointer is non-const (so it can be written to), the inner pointer is const.
 */
unsigned long pedigree_strtoul(const char *nptr, char const **endptr, int base);

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
