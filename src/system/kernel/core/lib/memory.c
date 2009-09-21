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

void *memset(void *buf, int c, size_t len)
{
  unsigned char *tmp = (unsigned char *)buf;
  while(len--)
  {
    *tmp++ = c;
  }
  return buf;
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
    char offset = (sizeof(size_t) - ((size_t)p1 % sizeof(size_t))) % sizeof(size_t);
    
    // see if it's even worth aligning
    if(n <= sizeof(size_t))
    {
        asm volatile("rep movsb;"::"D"(p1), "S"(p2), "c"(n));
        return s1;
    }
    
    // align p1 on a natural boundary
    asm volatile("rep movsb;":"=D"(p1), "=S"(p2):"D"(p1), "S"(p2), "c"(offset));
    n -= offset;
    
    // move in size_t size'd blocks
#if defined(X64)
    asm volatile("rep movsq;":"=D"(p1), "=S"(p2):"D"(p1), "S"(p2), "c"(n >> 3));
#elif defined(X86)
    asm volatile("rep movsl;":"=D"(p1), "=S"(p2):"D"(p1), "S"(p2), "c"(n >> 2));
#endif    
    
    // clean up the remaining bytes
    asm volatile("rep movsb;"::"D"(p1), "S"(p2), "c"(n % sizeof(size_t)));

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
