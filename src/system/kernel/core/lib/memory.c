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

void *memcpy(void *dest, const void *src, size_t len)
{
  const unsigned char *sp = (const unsigned char *)src;
  unsigned char *dp = (unsigned char *)dest;
  for (; len != 0; len--) *dp++ = *sp++;
  return dest;
}

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
