/*
 * Copyright (c) 2013 Matthew Iselin
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
#define _COMPILING_NEWLIB

#include "newlib.h"

#include <stdint.h>

#include <compiler.h>

// #define DEBUG_MEMSET

#ifdef DEBUG_MEMSET
#include <syslog.h>
#endif

/// \todo If we ever get an ARM newlib... we'll need some #ifdefs.

#define NATURAL_MASK        (sizeof(size_t) - 1)

void *memset(void *s, int c, size_t n)
{
    char *p1 = (char *) s;
    size_t unused;

#ifdef DEBUG_MEMSET
    syslog(LOG_INFO, "memset(%p, %d, %d)", s, c, n);
#endif

    // Align to a natural boundary.
    size_t align = sizeof(size_t) - (((uintptr_t) p1) & NATURAL_MASK);
    if(UNLIKELY(align > n)) align = n;
    switch(align)
    {
#ifdef X64
            case 7:
                *p1++ = c;
            case 6:
                *p1++ = c;
            case 5:
                *p1++ = c;
            case 4:
                *p1++ = c;
#endif
            case 3:
                *p1++ = c;
            case 2:
                *p1++ = c;
            case 1:
                *p1++ = c;
                n -= align;
            default:
                break;
    }

    if(UNLIKELY(!n))
        return s;

    // Load value. We would like to write a full size_t at a time if possible.
    // We special-case zero (a simple XOR), but for non-zero input, we set
    // the byte in all positions of a full size_t.
    size_t val = c;

#ifdef X64
    size_t blocks = n >> 3;
#else
    size_t blocks = n >> 2;
#endif

#ifdef DEBUG_MEMSET
    syslog(LOG_INFO, "  -> aligned %p, %d, %d", s, c, n);
#endif

    if(LIKELY(blocks))
    {
        if(val)
        {
#ifdef X64
            val = (val << 56) | (val << 48) | (val << 40) | (val << 32) | (val << 24) | (val << 16) | (val << 8) | val;
            asm volatile("rep; stosq" : "=D" (p1), "=c" (unused) : "D" (p1), "c" (blocks), "a" (val) : "memory");
#else
            val = (val << 24) | (val << 16) | (val << 8) | val;
            asm volatile("rep; stosl" : "=D" (p1), "=c" (unused) : "D" (p1), "c" (blocks), "a" (val) : "memory");
#endif
        }
        else
        {
#ifdef X64
            asm volatile("xor %%rax,%%rax; rep; stosq" : "=D" (p1), "=c" (unused) : "D" (p1), "c" (blocks) : "memory", "rax");
#else
            asm volatile("xor %%eax,%%eax; rep; stosl" : "=D" (p1), "=c" (unused) : "D" (p1), "c" (blocks) : "memory", "eax");
#endif
        }
    }

    // Tail.
    size_t tail = n & NATURAL_MASK;
#ifdef DEBUG_MEMSET
    syslog(LOG_INFO, "  -> tail %p, %d, %d", p1, c, tail);
#endif
    switch(tail)
    {
#ifdef X64
            case 7:
                *p1++ = c;
            case 6:
                *p1++ = c;
            case 5:
                *p1++ = c;
            case 4:
                *p1++ = c;
#endif
            case 3:
                *p1++ = c;
            case 2:
                *p1++ = c;
            case 1:
                *p1++ = c;
                n -= align;
            default:
                break;
    }

    return s;
}

