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


#include "support.h"

#pragma GCC optimize ("no-tree-loop-distribute-patterns")

extern "C"
{

int strncpy(char *dest, const char *src, int len)
{
  int i = 0;
  while (*src && len)
  {
    *dest++ = *src++;
    len--;
    i++;
  }
  *dest = '\0';
  return i;
}

int memset(void *buf, int c, size_t len)
{
  size_t count = len;
  char *tmp = (char *) buf;
  while(len--)
  {
    *tmp++ = (char) c;
  }
  return count;
}

void memcpy(void *dest, const void *src, size_t len)
{
  const unsigned char *sp = (const unsigned char *)src;
  unsigned char *dp = (unsigned char *)dest;
  for (; len != 0; len--) *dp++ = *sp++;
}

int strcmp(const char *p1, const char *p2)
{
  int i = 0;
  int failed = 0;
  while(p1[i] != '\0' && p2[i] != '\0')
  {
    if(p1[i] != p2[i])
    {
      failed = 1;
      break;
    }
    i++;
  }
  // why did the loop exit?
  if( (p1[i] == '\0' && p2[i] != '\0') || (p1[i] != '\0' && p2[i] == '\0') )
    failed = 1;

  return failed;
}

}
