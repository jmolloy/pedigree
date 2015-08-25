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

#define _COMPILING_NEWLIB

#include "newlib.h"

#include <stdint.h>

#include <compiler.h>

#pragma GCC optimize ("no-tree-loop-distribute-patterns")

/// \todo If we ever get an ARM newlib... we'll need some #ifdefs.

#ifdef X86_COMMON

#define SSE_ALIGN_SIZE      0x10
#define SSE_ALIGN_MASK      0xF
#define NATURAL_MASK        (sizeof(size_t) - 1)

#define SSE_THRESHOLD 524288 // 512 KiB

/// SSE2-optimised memcpy, where source is unaligned but destination is aligned.
static void *sse2_src_unaligned_memcpy(void *restrict s1, const void *restrict s2, const size_t n)
{
    uintptr_t p1 = (uintptr_t) s1;
    uintptr_t p2 = (uintptr_t) s2;

    // Count of 128 byte blocks
    size_t i = n >> 7;
    for(; i > 0; i--)
    {
        asm volatile("prefetchnta 128(%0); \
                      prefetchnta 192(%0); \
                      prefetchnta 256(%0); \
                      prefetchnta 320(%0); \
                      movdqu 0(%0), %%xmm0; \
                      movdqu 16(%0), %%xmm1; \
                      movdqu 32(%0), %%xmm2; \
                      movdqu 48(%0), %%xmm3; \
                      movdqu 64(%0), %%xmm4; \
                      movdqu 80(%0), %%xmm5; \
                      movdqu 96(%0), %%xmm6; \
                      movdqu 112(%0), %%xmm7; \
                      movntdq %%xmm0, 0(%1); \
                      movntdq %%xmm1, 16(%1); \
                      movntdq %%xmm2, 32(%1); \
                      movntdq %%xmm3, 48(%1); \
                      movntdq %%xmm4, 64(%1); \
                      movntdq %%xmm5, 80(%1); \
                      movntdq %%xmm6, 96(%1); \
                      movntdq %%xmm7, 112(%1);" : : "r" (p2), "r" (p1) : "memory");

        // Increment counters.
        p1 += 128;
        p2 += 128;
    }

    return s1;

}

/// SSE2-optimised memcpy, where s1 and s2 are both unaligned.
static void *sse2_unaligned_memcpy(void *restrict s1, const void *restrict s2, const size_t n)
{
    uintptr_t p1 = (uintptr_t) s1;
    uintptr_t p2 = (uintptr_t) s2;

    // Count of 128 byte blocks
    size_t i = n >> 7;
    for(; i > 0; i--)
    {
        asm volatile("prefetchnta 128(%0); \
                      prefetchnta 192(%0); \
                      prefetchnta 256(%0); \
                      prefetchnta 320(%0); \
                      movdqu 0(%0), %%xmm0; \
                      movdqu 16(%0), %%xmm1; \
                      movdqu 32(%0), %%xmm2; \
                      movdqu 48(%0), %%xmm3; \
                      movdqu 64(%0), %%xmm4; \
                      movdqu 80(%0), %%xmm5; \
                      movdqu 96(%0), %%xmm6; \
                      movdqu 112(%0), %%xmm7; \
                      movdqu %%xmm0, 0(%1); \
                      movdqu %%xmm1, 16(%1); \
                      movdqu %%xmm2, 32(%1); \
                      movdqu %%xmm3, 48(%1); \
                      movdqu %%xmm4, 64(%1); \
                      movdqu %%xmm5, 80(%1); \
                      movdqu %%xmm6, 96(%1); \
                      movdqu %%xmm7, 112(%1);" : : "r" (p2), "r" (p1) : "memory");

        // Increment counters.
        p1 += 128;
        p2 += 128;
    }

    return s1;
}

/// SSE2-optimised memcpy with fully aligned inputs.
static void *sse2_aligned_memcpy(void *restrict s1, const void *restrict s2, const size_t n)
{
    uintptr_t p1 = (uintptr_t) s1;
    uintptr_t p2 = (uintptr_t) s2;

    // Count of 128 byte blocks
    size_t i = n >> 7;
    for(; i > 0; i--)
    {
        asm volatile("prefetchnta 128(%0);" \
                     "prefetchnta 192(%0);" \
                     "prefetchnta 256(%0);" \
                     "prefetchnta 320(%0);" \
                     "movdqa 0(%0), %%xmm0;" \
                     "movdqa 16(%0), %%xmm1;" \
                     "movdqa 32(%0), %%xmm2;" \
                     "movdqa 48(%0), %%xmm3;" \
                     "movdqa 64(%0), %%xmm4;" \
                     "movdqa 80(%0), %%xmm5;" \
                     "movdqa 96(%0), %%xmm6;" \
                     "movdqa 112(%0), %%xmm7;" \
                     "movntdq %%xmm0, 0(%1);" \
                     "movntdq %%xmm1, 16(%1);" \
                     "movntdq %%xmm2, 32(%1);" \
                     "movntdq %%xmm3, 48(%1);" \
                     "movntdq %%xmm4, 64(%1);" \
                     "movntdq %%xmm5, 80(%1);" \
                     "movntdq %%xmm6, 96(%1);" \
                     "movntdq %%xmm7, 112(%1);" : : "r" (p2), "r" (p1) : "memory");

        // Increment counters.
        p1 += 128;
        p2 += 128;
    }

    return s1;
}


void *memcpy(void *restrict s1, const void *restrict s2, size_t n)
{
    char *restrict p1 = (char *)s1;
    const char *restrict p2 = (const char *)s2;

    uintptr_t p1_u = (uintptr_t) p1;
    uintptr_t p2_u = (uintptr_t) p2;

    // Check for bad usage of memcpy
    if(UNLIKELY(!n)) return s1;

    size_t unused;

    // Should we do SSE?
    // rep movs* is VERY fast until the point at which SSE prefetches
    // and the like can actually greatly improve performance.
    if(n > SSE_THRESHOLD)
    {
        // Transfer in 128 byte blocks.
        uintptr_t p1Align = p1_u & SSE_ALIGN_MASK;
        uintptr_t p2Align = p2_u & SSE_ALIGN_MASK;
        if(LIKELY(!(p1Align || p2Align)))
        {
            // Fully aligned inputs.
            /// \note Please call memcpy with aligned src/dest as much as
            ///       possible, as the performance boost is quite nice.
            sse2_aligned_memcpy(p1, p2, n);
        }
        else
        {
            // Align destination, use source unaligned.
            // This allows us to use aligned non-temporal writes, with
            // (prefetched) unaligned reads from the source.
            size_t distance = SSE_ALIGN_SIZE - p1Align;
            switch(distance)
            {
                case 15:
                    *p1++ = *p2++;
                case 14:
                    *p1++ = *p2++;
                case 13:
                    *p1++ = *p2++;
                case 12:
                    *p1++ = *p2++;
                case 11:
                    *p1++ = *p2++;
                case 10:
                    *p1++ = *p2++;
                case 9:
                    *p1++ = *p2++;
                case 8:
                    *p1++ = *p2++;
                case 7:
                    *p1++ = *p2++;
                case 6:
                    *p1++ = *p2++;
                case 5:
                    *p1++ = *p2++;
                case 4:
                    *p1++ = *p2++;
                case 3:
                    *p1++ = *p2++;
                case 2:
                    *p1++ = *p2++;
                case 1:
                    *p1++ = *p2++;
                default:
                    break;
            }

            sse2_src_unaligned_memcpy(p1, p2, n);
        }

        size_t count = n & ~((1 << 7) - 1); // Aligns n to 128 byte boundary.
        p1 += count;
        p2 += count;

        // Finish off any trailing bytes that couldn't be transferred as part of a 128 byte block.
        if(n != count)
            memcpy(p1, p2, n - count);

        return s1;
    }
    else
    {
        // Not enough bytes for SSE to be worth the FPU save.

        // See if it's even worth aligning
        switch(n)
        {
#ifdef X64
            case 7:
                *p1++ = *p2++;
            case 6:
                *p1++ = *p2++;
            case 5:
                *p1++ = *p2++;
            case 4:
                *p1++ = *p2++;
#endif
            case 3:
                *p1++ = *p2++;
            case 2:
                *p1++ = *p2++;
            case 1:
                *p1++ = *p2++;
                return s1;
            default:
                break;
        }

        // calculate the distance to the nearest natural boundary
        size_t offset = (sizeof(size_t) - (((size_t) p1) & NATURAL_MASK)) & NATURAL_MASK;

        // Align p1 on a natural boundary
        switch(offset)
        {
#ifdef X64
            case 7:
                *p1++ = *p2++;
            case 6:
                *p1++ = *p2++;
            case 5:
                *p1++ = *p2++;
            case 4:
                *p1++ = *p2++;
#endif
            case 3:
                *p1++ = *p2++;
            case 2:
                *p1++ = *p2++;
            case 1:
                *p1++ = *p2++;
                n -= offset;
            default:
                break;
        }

        // Move in size_t size'd blocks
#ifdef X64
        size_t blocks = n >> 3;
#else
        size_t blocks = n >> 2;
#endif
        if(LIKELY(blocks))
        {
#ifdef X64
            asm volatile("rep movsq;":"=D"(p1), "=S"(p2), "=c"(unused):"D"(p1), "S"(p2), "c"(blocks) : "memory");
#else
            asm volatile("rep movsl;":"=D"(p1), "=S"(p2), "=c"(unused):"D"(p1), "S"(p2), "c"(blocks) : "memory");
#endif
        }

        // Clean up the remaining bytes
        size_t tail = n & NATURAL_MASK;
        switch(tail)
        {
#ifdef X64
            case 7:
                *p1++ = *p2++;
            case 6:
                *p1++ = *p2++;
            case 5:
                *p1++ = *p2++;
            case 4:
                *p1++ = *p2++;
#endif
            case 3:
                *p1++ = *p2++;
            case 2:
                *p1++ = *p2++;
            case 1:
                *p1++ = *p2++;
            default:
                break;
        }
    }

    return s1;
}

#else

/* No custom memcpy on ARM. */

#endif

