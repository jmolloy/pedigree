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
#include <utilities/assert.h>
#include <compiler.h>

#include <panic.h>

#ifndef __clang__
#pragma GCC optimize ("no-tree-loop-distribute-patterns")
#endif

/**
    x86 note:
    Pedigree requires at least an SSE2-capable CPU in order to run. This allows
    us to make assumptions here about CPU support of certain features.
    
    Eventually we will have copies of each function for SSE2,3,4,x.
**/

#define SSE_ALIGN_SIZE      0x10
#define SSE_ALIGN_MASK      0xF
#define NATURAL_MASK        (sizeof(size_t) - 1)

// Below about this size, rep movs{d,q} is in fact faster than SSE.
#define SSE_THRESHOLD 524288 // 512 KiB

// Set to 1 to allow SSE to be used.
#define USE_SSE             0

#define USE_SSE_MEMCPY      1
#define USE_SSE_MEMSET      1

// Condense X86-ish systems into one define for this file.
#if defined(X86_COMMON) || defined(HOSTED_X86_COMMON)
#define IS_X86
#endif

#ifdef HOSTED_X64
#define X64
#endif

#ifdef IS_X86
static void *memset_nonzero(void *buf, int c, size_t n)
{
    char *p = (char *)buf;

    // check for bad usage of memcpy
    if(!n) return buf;

    c &= 0xFF;

    size_t unused;
    if(n == 1)
    {
        *p = c;
        return buf;
    }

    while(n)
    {
#ifdef BITS_64
        if((n % 8) == 0)
        {
            uint64_t c64 = c;
            uint64_t word = (c64 << 56ULL) | (c64 << 48ULL) | (c64 << 40ULL) | (c64 << 32ULL) | (c64 << 24ULL) | (c64 << 16ULL) | (c64 << 8ULL) | c64;
            asm volatile("rep stosq" : "=c" (unused) : "D" (p), "c" (n / 8), "a" (word));
            n = 0;
        }
        else
#endif
        if((n % 4) == 0)
        {
            uint32_t c32 = c;
            uint32_t word = (c32 << 24U) | (c32 << 16U) | (c32 << 8U) | c32;
            asm volatile("rep stosl" : "=c" (unused) : "D" (p), "c" (n / 4), "a" (word));
            n = 0;
        }
        else if((n % 2) == 0)
        {
            uint16_t c16 = c;
            uint16_t word = (c16 << 8) | c16;
            asm volatile("rep stosw" : "=c" (unused) : "D" (p), "c" (n / 2), "a" (word));
            n = 0;
        }
        else
        {
            // Set until n is aligned in a friendly way.
            size_t nBytes = n;
            size_t q = 8 - (n % 8), d = 4 - (n % 4), w = 2 - (n % 2);
            if(n > 8)
            {
#ifdef BITS_64
                if((q < d) && (q < w))
                    nBytes = q;
                else
#endif
                if(d < w)
                    nBytes = d;
                else if(w < d)
                    nBytes = w;
            }

            asm volatile("rep stosb;":"=c"(unused):"D"(p), "c"(nBytes), "a"(c));
            n -= nBytes;
            p += nBytes;
        }
    }

    return buf;
}

void *memset(void *buf, int c, size_t n)
{
#if USE_SSE && USE_SSE_MEMSET
    if(c)
        return memset_nonzero(buf, c, n);
    else
    {
        uintptr_t p = (uintptr_t) buf;
        size_t off = 0;
        
        if(!n)
            return buf;
        
        // Align to the SSE block size
        if(p & SSE_ALIGN_MASK)
        {
            // If p == 0x01, nForAlignment will be 0x0F (0x10 - 0x01).
            // p + off = 0x01 + 0x0F = 0x10
            size_t nForAlignment = (SSE_ALIGN_SIZE - (p & SSE_ALIGN_MASK));
            if(nForAlignment > n)
                nForAlignment = n;
            
            uint32_t unused = 0; // Used to tell GCC ECX gets smashed.
            off = nForAlignment;
            asm volatile("rep stosb" : "=c" (unused) : "D" (buf), "c" (nForAlignment), "a" (0));
            
            if(off >= n)
                return buf;
        }
        
        // Clearing memory to zero, and at least one SSE-size block!
        // Massive optimisation possible!
        
        // Now aligned, clear 16 bytes at a time
        asm volatile("pxor %xmm0, %xmm0");
        for(; (off + SSE_ALIGN_SIZE) <= n; off += SSE_ALIGN_SIZE)
        {
            // assert(((p + off) & SSE_ALIGN_MASK) == 0);
            asm volatile("movdqa %%xmm0, (%0)" : : "r" (p + off));
        }
        
        // Any remaining bytes can now be cleared.
        if(off < n)
            asm volatile("rep stosb" : : "D" (p + off), "c" (n - off), "a" (0));
        
        return buf;
    }
#else
    return memset_nonzero(buf, c, n);
#endif
}

#else
void *memset(void *buf, int c, size_t len)
{
  unsigned char *tmp = (unsigned char *)buf;
  for(size_t i = 0; i < len; ++i)
  {
    *tmp++ = c;
  }
  return buf;
}
#endif

#ifdef IS_X86
void *wmemset(void *buf, int c, size_t n)
{
#if USE_SSE && USE_SSE_MEMSET
    // Use SSE if it would give us an advantage here.
    if((!c) && ((n << 1) >= SSE_ALIGN_SIZE))
      return memset(buf, (char) c, n << 1);
#endif
    
    char *p = (char *)buf;

    // check for bad usage of memcpy
    if(!n) return buf;

    size_t unused;
    // see if it's even worth aligning
    asm volatile("rep stosw;":"=c"(unused):"D"(p), "c"(n), "a"(c));
    return buf;
}
#else
void *wmemset(void *buf, int c, size_t len)
{
  unsigned short *tmp = (unsigned short *)buf;
  while(len--)
  {
    *tmp++ = c;
  }
  return buf;
}
#endif

#ifdef IS_X86
void *dmemset(void *buf, unsigned int c, size_t n)
{
#if USE_SSE && USE_SSE_MEMSET
    // Use SSE if it would give us an advantage here.
    if((!c) && ((n << 2) >= SSE_ALIGN_SIZE))
      return memset(buf, (char) c, n << 2);
#endif
  
    char *p = (char *)buf;

    // check for bad usage of memcpy
    if(!n) return buf;

    size_t unused;
    // see if it's even worth aligning
    asm volatile("rep stosl;":"=c"(unused):"D"(p), "c"(n), "a"(c));
    return buf;
}
#else
void *dmemset(void *buf, unsigned int c, size_t len)
{
  unsigned int *tmp = (unsigned int *)buf;
  while(len--)
  {
    *tmp++ = c;
  }
  return buf;
}
#endif

void *qmemset(void *buf, unsigned long long c, size_t len)
{
#ifdef IS_X86

#if USE_SSE && USE_SSE_MEMSET
  // Use SSE if it would give us an advantage here.
  if((!c) && ((len << 3) >= SSE_ALIGN_SIZE))
    return memset(buf, (char) c, len << 3);
#endif

#endif
  
#ifdef X64
  asm volatile("rep stosq" :: "a" (c), "c" (len), "D" (buf));
  return buf;
#else
  unsigned long long *p = (unsigned long long*) buf;
  while(len--)
    *p++ = c;
  return buf;
#endif
}

#ifdef IS_X86

#define SSE2_SAVE_REGION_SIZE   (512)
#define SSE2_SAVE_REGION_ALIGN  __attribute__((aligned(16)))

/// \todo sse2_save/restore should probably use fxsave/fxrstor...

/// SSE2 register save.
void sse2_save(char p[SSE2_SAVE_REGION_SIZE])
{
    asm volatile("movdqu %%xmm0, 0(%0); \
                  movdqu %%xmm1, 16(%0); \
                  movdqu %%xmm2, 32(%0); \
                  movdqu %%xmm3, 48(%0); \
                  movdqu %%xmm4, 64(%0); \
                  movdqu %%xmm5, 80(%0); \
                  movdqu %%xmm6, 96(%0); \
                  movdqu %%xmm7, 112(%0);" :: "r" (p) : "memory");
}

/// SSE2 register save.
void sse2_restore(char p[SSE2_SAVE_REGION_SIZE])
{
    asm volatile("movdqu 0(%0), %%xmm0; \
                  movdqu 16(%0), %%xmm1; \
                  movdqu 32(%0), %%xmm2; \
                  movdqu 48(%0), %%xmm3; \
                  movdqu 64(%0), %%xmm4; \
                  movdqu 80(%0), %%xmm5; \
                  movdqu 96(%0), %%xmm6; \
                  movdqu 112(%0), %%xmm7;" :: "r" (p) : "memory");
}

/// SSE2-optimised memcpy, where source is unaligned but destination is aligned.
void *sse2_src_unaligned_memcpy(void *restrict s1, const void *restrict s2, const size_t n)
{
    uintptr_t p1 = (uintptr_t) s1;
    uintptr_t p2 = (uintptr_t) s2;

    if(UNLIKELY(!n)) return s1;

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
void *sse2_unaligned_memcpy(void *restrict s1, const void *restrict s2, const size_t n)
{
    uintptr_t p1 = (uintptr_t) s1;
    uintptr_t p2 = (uintptr_t) s2;

    if(UNLIKELY(!n)) return s1;

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
void *sse2_aligned_memcpy(void *restrict s1, const void *restrict s2, const size_t n)
{
    uintptr_t p1 = (uintptr_t) s1;
    uintptr_t p2 = (uintptr_t) s2;

    if(UNLIKELY(!n)) return s1;

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

    size_t orig_n = n;

    size_t unused;

    // Check for bad usage of memcpy
    if(UNLIKELY(!n)) return s1;

#if USE_SSE
    static int can_sse = 0;

    // Check to see if we can now toggle on SSE.
    if(UNLIKELY(!can_sse))
    {
        uintptr_t cr0, cr4;
        asm volatile("mov %%cr0, %0" : "=r" (cr0));
        if((cr0 & (1 << 2)) == 0)
        {
            // EM in CR0 unset. Good to go.
            can_sse = 1;
        }
    }

    // Should we do SSE? Note that SSE involves an FPU state save,
    // so it must be REALLY worth it.
    if(can_sse && (n > SSE_THRESHOLD))
    {
        /// \todo We should NOT save SSE state if the process has not used SSE...
        char save_state[SSE2_SAVE_REGION_SIZE] SSE2_SAVE_REGION_ALIGN;

        long ints = 0;
        asm volatile("pushf; pop %0; and $0x200, %0; cli" : "=r" (ints));
        // Save SSE registers, as we are about to destroy them.
        sse2_save(save_state);

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
                    n -= distance;
                default:
                    break;
            }

            sse2_src_unaligned_memcpy(p1, p2, n);
        }

        size_t count = n & ~127; // Aligns n to 128 byte boundary.
        p1 += count;
        p2 += count;

        // Finish off any trailing bytes that couldn't be transferred as part of a 128 byte block.
        memcpy(p1, p2, n & 127);

        /// \todo FPU restore here!
        sse2_restore(save_state);
        if(ints)
            asm volatile("sti");

        return s1;
    }
    else
#endif
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

void *memcpy_gcc(void *restrict s1, const void *restrict s2, size_t n)
{
    char *restrict p1 = s1;
    const char *restrict p2 = s2;
    while(n--)
        *p1++ = *p2++;
    return s1;
}

#else
void *memcpy(void *dest, const void *src, size_t len)
{
  const unsigned char *sp = (const unsigned char *)src;
  unsigned char *dp = (unsigned char *)dest;
  for (; len != 0; len--) *dp++ = *sp++;
  return dest;
}
#endif

// #ifdef IS_X86
#if 0
void *memmove(void *s1, const void *s2, size_t n)
{
    char *p1 = (char *)s1;
    const char *p2 = (const char *)s2;

    // Don't use SSE for now.
#if 0
    asm volatile("  prefetchnta 0(%0);  \
                    prefetchnta 32(%0); "
                 ::"r"(s1));
#endif
    // check for bad usage of memmove
    if(!n) return s1;

    size_t offset;
    if (s1 <= s2)
    {
        asm volatile("cld");
        // calculate the distance to the nearest natural boundary
        offset = (sizeof(size_t) - ((size_t)p1 % sizeof(size_t))) % sizeof(size_t);
    }
    else
    {
        // Work backwards.
        p1 += n;
        p2 += n;
        // calculate the distance to the nearest natural boundary
        offset = ((size_t)p1+n) % sizeof(size_t);
        asm volatile("std");
    }

    size_t unused;
    // see if it's even worth aligning
    if(n <= sizeof(size_t))
    {
        asm volatile("rep movsb;":"=c"(unused):"D"(p1), "S"(p2), "c"(n));
        return s1;
    }

    n -= offset;

    // align p1 on a natural boundary
    asm volatile("rep movsb;":"=D"(p1), "=S"(p2), "=c"(unused):"D"(p1), "S"(p2), "c"(offset));

    // move in size_t size'd blocks
#if defined(X64)
    asm volatile("rep movsq;":"=D"(p1), "=S"(p2), "=c"(unused):"D"(p1), "S"(p2), "c"(n >> 3));
#elif defined(X86)
    asm volatile("rep movsl;":"=D"(p1), "=S"(p2), "=c"(unused):"D"(p1), "S"(p2), "c"(n >> 2));
#endif

    // clean up the remaining bytes
    asm volatile("rep movsb;":"=c"(unused):"D"(p1), "S"(p2), "c"(n % sizeof(size_t)));

    if (s1 > s2)
        asm volatile("cld");

    return s1;
}
#else
void *memmove(void *s1, const void *s2, size_t n)
{
  if (s1 <= s2)
    memcpy(s1, s2, n);
  else
  {
    const unsigned char *sp = (const unsigned char *) s2 + n;
    unsigned char *dp = (unsigned char *) s1 + n;
    for (; n != 0; n--) *dp-- = *sp--;
  }
  return s1;
}
#endif

int memcmp(const void *p1, const void *p2, size_t len)
{
    const char* a = (const char*) p1;
    const char* b = (const char*) p2;
    size_t i = 0;
    for(; i < len; i++)
    {
        if(a[i] < b[i])
            return -1;
        else if(a[i] > b[i])
            return 1;
    }
    return 0;
}
