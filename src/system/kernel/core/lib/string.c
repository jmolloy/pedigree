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

#include <stdarg.h>
#include <stddef.h>
#include <utilities/utility.h>
#include <processor/types.h>

#define ULONG_MAX -1

size_t _StringLength(const char *src)
{
  const char *orig = src;
  while (*src) ++src;
  return src - orig;
}

char *StringCopy(char *dest, const char *src)
{
  while (*src)
  {
    *dest = *src;
    ++dest; ++src;
  }
  *dest = '\0';

  return dest;
}

char *StringCopyN(char *dest, const char *src, size_t len)
{
  const char *orig_dest = dest;
  for (size_t i = 0; i < len; ++i)
  {
    if (*src)
      *dest++ = *src++;
    else
      break;
  }
  *dest = '\0';

  return dest;
}

int StringFormat(char *buf, const char *fmt, ...)
{
  va_list args;
  int i;

  va_start(args, fmt);
  i = VStringFormat(buf, fmt, args);
  va_end(args);

  return i;
}

int StringCompare(const char *p1, const char *p2)
{
  size_t i = 0;
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

int StringCompareN(const char *p1, const char *p2, size_t n)
{
  size_t i = 0;
  int failed = 0;
  while(p1[i] != '\0' && p2[i] != '\0' && n)
  {
    if(p1[i] != p2[i])
    {
      failed = 1;
      break;
    }
    i++;
    n--;
  }
  // why did the loop exit?
  if( n && ( (p1[i] == '\0' && p2[i] != '\0') || (p1[i] != '\0' && p2[i] == '\0') ) )
    failed = 1;

  return failed;
}

char *StringConcat(char *dest, const char *src)
{
  size_t di = StringLength(dest);
  size_t si = 0;
  while (src[si])
    dest[di++] = src[si++];
  
  dest[di] = '\0';

  return dest;
}

char *StringConcatN(char *dest, const char *src, size_t n)
{
  size_t di = StringLength(dest);
  size_t si = 0;
  while (src[si] && n)
  {
    dest[di++] = src[si++];
    n--;
  }
  
  dest[di] = '\0';

  return dest;
}

int isspace(int c)
{
  return (c == ' ' || c == '\n' || c == '\r' || c == '\t');
}

int isupper(int c)
{
  return (c >= 'A' && c <= 'Z');
}

int islower(int c)
{
  return (c >= 'a' && c <= 'z');
}

int isdigit(int c)
{
  return (c >= '0' && c <= '9');
}

int isalpha(int c)
{
  return isupper(c) || islower(c) || isdigit(c);
}

unsigned long StringToUnsignedLong(const char *nptr, char const **endptr, int base)
{
  register const char *s = nptr;
  register unsigned long acc;
  register int c;
  register unsigned long cutoff;
  register int neg = 0, any, cutlim;

  /*
   * See strtol for comments as to the logic used.
   */
  do {
    c = *s++;
  } while (isspace(c));
  if (c == '-') {
    neg = 1;
    c = *s++;
  } else if (c == '+')
          c = *s++;
  if ((base == 0 || base == 16) &&
    c == '0' && (*s == 'x' || *s == 'X')) {
      c = s[1];
      s += 2;
      base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;
  cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
  cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
  for (acc = 0, any = 0;; c = *s++) {
    if (isdigit(c))
            c -= '0';
    else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else
            break;
    if (c >= base)
            break;
  if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0) {
    acc = ULONG_MAX;
  } else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = (const char *) (any ? s - 1 : nptr);

  return (acc);
}

const char *StringFind(const char *str, int target)
{
  while (*str)
  {
    if (*str == target)
    {
      return str;
    }

    ++str;
  }

  return NULL;
}

const char *StringReverseFind(const char *str, int target)
{
  // StringLength must traverse the entire string once to find the length,
  // so rather than finding the length and then traversing in reverse, we just
  // traverse the string once. This gives a small performance boost.
  const char *found = NULL;
  while (*str)
  {
    if (*str == target)
    {
      found = str;
    }

    ++str;
  }

  return found;
}

// Provide forwarding functions to handle GCC optimising things.
size_t strlen(const char *s)
{
  return StringLength(s);
}

char *strcpy(char *dest, const char *src)
{
  return StringCopy(dest, src);
}

char *strncpy(char *dest, const char *src, size_t len)
{
  return StringCopyN(dest, src, len);
}

int strcmp(const char *p1, const char *p2)
{
  return StringCompare(p1, p2);
}

int strncmp(const char *p1, const char *p2, size_t n)
{
  return StringCompareN(p1, p2, n);
}

char *strcat(char *dest, const char *src)
{
  return StringConcat(dest, src);
}

char *strncat(char *dest, const char *src, size_t n)
{
  return StringConcatN(dest, src, n);
}

const char *strchr(const char *str, int target)
{
  return StringFind(str, target);
}

const char *strrchr(const char *str, int target)
{
  return StringReverseFind(str, target);
}

int vsprintf(char *buf, const char *fmt, va_list arg)
{
  return VStringFormat(buf, fmt, arg);
}

unsigned long strtoul(const char *nptr, char const **endptr, int base)
{
  return StringToUnsignedLong(nptr, endptr, base);
}
