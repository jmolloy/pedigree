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

#include <stdarg.h>

int strlen(const char *src)
{
  int i = 0;
  while (*src++) i++;
  return i;
}

int strcpy(char *dest, const char *src)
{
  while (*src)
    *dest++ = *src++;
  *dest = '\0';
}

int strncpy(char *dest, const char *src, int len)
{
  while (*src && len)
  {
    *dest++ = *src++;
    len--;
  }
  *dest = '\0';
}

int sprintf(char *buf, const char *fmt, ...)
{
  va_list args;
  int i;

  va_start(args, fmt);
  i = vsprintf(buf, fmt, args);
  va_end(args);

  return i;
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

int strncmp(const char *p1, const char *p2, int n)
{
  int i = 0;
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

char *strcat(char *dest, const char *src)
{
  int di = strlen(dest);
  int si = 0;
  while (src[si])
    dest[di++] = src[si++];
  
  dest[di] = '\0';
}

char *strncat(char *dest, const char *src, int n)
{
  int di = strlen(dest);
  int si = 0;
  while (src[si] && n)
  {
    dest[di++] = src[si++];
    n--;
  }
  
  dest[di] = '\0';
}

int isspace(char c)
{
  return (c == ' ' || c == '\n' || c == '\r' || c == '\t');
}

int isupper(char c)
{
  return (c >= 'A' && c <= 'Z');
}

int islower(char c)
{
  return (c >= 'a' && c <= 'z');
}

int isdigit(char c)
{
  return (c >= '0' && c <= '9');
}

int isalpha(char c)
{
  return isupper(c) || islower(c) || isdigit(c);
}

#define ULONG_MAX -1
unsigned long strtoul(const char *nptr, char **endptr, int base)
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
    *endptr = (char *) (any ? s - 1 : nptr);

  return (acc);
}

char *strrchr(const char *str, char target)
{
  int i;
  for (i = strlen(str); i >= 0; i--)
    if (str[i] == target)
      return &str[i];
  return (char*)0;
}