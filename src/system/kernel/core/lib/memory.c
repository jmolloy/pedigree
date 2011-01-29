/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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

#include <panic.h>

/**
    x86 note:
    Pedigree requires at least an SSE2-capable CPU in order to run. This allows
    us to make assumptions here about CPU support of certain features.
    
    Eventually we will have copies of each function for SSE2,3,4,x.
**/

#define SSE_ALIGN_SIZE      0x10
#define SSE_ALIGN_MASK      0xF

// Set to 1 to allow SSE to be used.
#define USE_SSE             1

#ifdef X86_COMMON
inline void *memset_nonzero(void *buf, int c, size_t n)
{
    char *p = (char *)buf;

    // check for bad usage of memcpy
    if(!n) return buf;

    size_t unused;
    // see if it's even worth aligning
    asm volatile("rep stosb;":"=c"(unused):"D"(p), "c"(n), "a"(c));
    return buf;
}

void *memset(void *buf, int c, size_t n)
{
#if USE_SSE
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
  while(len--)
  {
    *tmp++ = c;
  }
  return buf;
}
#endif

#ifdef X86_COMMON
void *wmemset(void *buf, int c, size_t n)
{
#if USE_SSE
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

#ifdef X86_COMMON
void *dmemset(void *buf, unsigned int c, size_t n)
{
#if USE_SSE
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
#ifdef X86_COMMON

#if USE_SSE
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

#ifdef X86_COMMON
/** This function courtesy of Josh Cornutt - cheers! */
void *memcpy(void *restrict s1, const void *restrict s2, size_t n)
{
    char *p1 = (char *)s1;
    const char *p2 = (const char *)s2;

    // Don't use SSE for now.
#if 0
    asm volatile("  prefetchnta 0(%0);  \
                    prefetchnta 32(%0); "
                 ::"r"(s1));
#endif
    // check for bad usage of memcpy
    if(!n) return s1;

    // calculate the distance to the nearest natural boundary
    size_t offset = (sizeof(size_t) - ((size_t)p1 % sizeof(size_t))) % sizeof(size_t);

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

    return s1;
}

void *memcpy_gcc(void *restrict s1, const void *restrict s2, size_t n)
{
    char *restrict p1 = s1;
    const char *restrict p2 = s2;
    while(n)
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

#ifdef X86_COMMON
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
    uint8_t *dest8 = (uint8_t*)s1 + n;
    const uint8_t *src8 = (const uint8_t*)s2 + n;

    while ((((uintptr_t)dest8 - 1) % 4) != 0 && n != 0)
    {
      *--dest8 = *--src8;
      --n;
    }

    uint32_t *dest32 = (uint32_t*)dest8;
    const uint32_t *src32 = (const uint32_t*)src8;
    while (n >= 4)
    {
      *--dest32 = *--src32;
      n -= 4;
    }

    dest8 = (uint8_t*)dest32;
    src8 = (const uint8_t*)src32;
    while (n != 0)
    {
      *--dest8 = *--src8;
      --n;
    }
  }
  return s1;
}

int memcmp(const void *p1, const void *p2, size_t len)
{
    size_t i = len - 1;
    const char* a = (const char*) p1;
    const char* b = (const char*) p2;
    for(; i >= 0; i--)
    {
        if(a[i] < b[i])
            return -1;
        else if(a[i] > b[i])
            return 1;
    }
    return 0;
}
#endif
