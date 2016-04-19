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

#undef memcpy

// Condense X86-ish systems into one define for this file.
/// \note this will break testsuite/hosted builds on non-x86 hosts.
#if defined(X86_COMMON) || defined(HOSTED_X86_COMMON) || defined(UTILITY_LINUX)
#define IS_X86
#endif

#ifdef HOSTED_X64
#define X64
#endif

int memcmp(const void *p1, const void *p2, size_t len) PURE;
void *memset(void *buf, int c, size_t n);
void *WordSet(void *buf, int c, size_t n);
void *DoubleWordSet(void *buf, unsigned int c, size_t n);
void *QuadWordSet(void *buf, unsigned long long c, size_t n);

void *memcpy(void *restrict s1, const void *restrict s2, size_t n);
void *memmove(void *s1, const void *s2, size_t n);

// asan provides a memcpy/memset/etc that we care about more than our custom
// ones, in general.
#ifndef HAS_ADDRESS_SANITIZER

int memcmp(const void *p1, const void *p2, size_t len)
{
    const char* a = (const char*) p1;
    const char* b = (const char*) p2;
    size_t i = 0;
    int r = 0;
    for(; i < len; i++)
    {
        if ((r = a[i] - b[i]) != 0)
          break;
    }
    return r;
}

void *memset(void *buf, int c, size_t n)
{
#ifdef IS_X86
    int a, b;
    asm volatile("rep stosb" : "=&D" (a), "=&c" (b) : "0" (buf), "a" (c), "1" (n) : "memory");
    return buf;
#else
    unsigned char *tmp = (unsigned char *)buf;
    for(size_t i = 0; i < n; ++i)
    {
      *tmp++ = c;
    }
    return buf;
#endif
}

void *memcpy(void *restrict s1, const void *restrict s2, size_t n)
{
#ifdef IS_X86
    int a, b, c;
    asm volatile("rep movsb" : "=&c" (a), "=&D" (b), "=&S" (c): "1" (s1), "2" (s2), "0" (n) : "memory");
    return s1;
#else
    const unsigned char *restrict sp = (const unsigned char *restrict)s2;
    unsigned char *restrict dp = (unsigned char *restrict)s1;
    for (; n != 0; n--) *dp++ = *sp++;
    return s1;
#endif
}

static int overlaps(const void *restrict s1, const void *restrict s2, size_t n) PURE;
static int overlaps(const void *restrict s1, const void *restrict s2, size_t n)
{
  uintptr_t a = (uintptr_t) s1;
  uintptr_t a_end = (uintptr_t) s1 + n;
  uintptr_t b = (uintptr_t) s2;
  uintptr_t b_end = (uintptr_t) s2 + n;

  return (a <= b_end) && (b <= a_end) ? 1 : 0;
}

void *memmove(void *s1, const void *s2, size_t n)
{
  if (UNLIKELY(!n)) return s1;

  const size_t orig_n = n;
  if (LIKELY((s1 < s2) || !overlaps(s1, s2, n)))
  {
    // No overlap, or there's overlap but we can copy forwards.
    memcpy(s1, s2, n);
  }
  else
  {
    // Writing bytes from s2 into s1 cannot be done forwards, use memmove.
    const unsigned char *sp = (const unsigned char *) s2 + (n - 1);
    unsigned char *dp = (unsigned char *) s1 + (n - 1);
    for (; n != 0; n--) *dp-- = *sp--;
  }

#ifdef ADDITIONAL_CHECKS
    // We can't memcmp if the regions overlap at all.
    if (LIKELY(!overlaps(s1, s2, orig_n)))
    {
      assert(!memcmp(s1, s2, orig_n));
    }
#endif

  return s1;
}

#endif  // HAS_ADDRESS_SANITIZER

void *WordSet(void *buf, int c, size_t n)
{
#ifdef IS_X86
    int a, b;
    asm volatile("rep stosw" : "=&D" (a), "=&c" (b) : "0" (buf), "a" (c), "1" (n) : "memory");
    return buf;
#else
    unsigned short *tmp = (unsigned short *)buf;
    while(n--)
    {
      *tmp++ = c;
    }
    return buf;
#endif
}

void *DoubleWordSet(void *buf, unsigned int c, size_t n)
{
#ifdef IS_X86
    int a, b;
    asm volatile("rep stosl" : "=&D" (a), "=&c" (b) : "0" (buf), "a" (c), "1" (n) : "memory");
    return buf;
#else
    unsigned int *tmp = (unsigned int *)buf;
    while(n--)
    {
      *tmp++ = c;
    }
    return buf;
#endif
}

void *QuadWordSet(void *buf, unsigned long long c, size_t n)
{
#ifdef X64
    int a, b;
    asm volatile("rep stosq" : "=&D" (a), "=&c" (b) : "0" (buf), "a" (c), "1" (n) : "memory");
    return buf;
#else
    unsigned long long *p = (unsigned long long*) buf;
    while(n--)
      *p++ = c;
    return buf;
#endif
}

// We still need memcpy etc as linked symbols for GCC optimisations, but we
// don't have to have their prototypes widely available. So, we implement our
// main functions in terms of the base calls.
void *ForwardMemoryCopy(void *a, const void *b, size_t c)
{
  return memcpy(a, b, c);
}

void *MemoryCopy(void *a, const void *b, size_t c)
{
  return memmove(a, b, c);
}

void *ByteSet(void *a, int b, size_t c)
{
  return memset(a, b, c);
}

int MemoryCompare(const void *a, const void *b, size_t c)
{
  return memcmp(a, b, c);
}
