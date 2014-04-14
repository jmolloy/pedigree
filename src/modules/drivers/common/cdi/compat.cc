/*
 * 
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

#include <processor/types.h>
#include <utilities/utility.h>
#include <Log.h>

#include "include/stdio.h"

int printf(const char* fmt, ...)
{
    static char print_temp[1024];
    va_list argptr;
    va_start(argptr, fmt);
    int ret = vsprintf(print_temp, fmt, argptr);
    if(print_temp[ret - 1] == '\n')
        print_temp[--ret] = 0;
    NOTICE(print_temp);
    va_end(argptr);
    return ret;
}

int snprintf(char *s, size_t n, const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    int ret = vsprintf(s, fmt, argptr);
    va_end(argptr);
    return ret;
}

int sprintf(char *s, const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    int ret = vsprintf(s, fmt, argptr);
    va_end(argptr);
    return ret;
}

int puts(const char *s)
{
    NOTICE(s);
    return strlen(s);
}
