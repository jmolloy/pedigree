/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

//
// Should only be included from ./syscall.h. This contains the syscall functions.
//

static int syscall0(int function)
{
  int eax = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  asm volatile("int $255" : "=a" (ret), "=b" (errno) : "0" (eax));
  return ret;
}

static int syscall1(int function, int p1)
{
  int eax = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  asm volatile("int $255" : "=a" (ret), "=b" (errno) : "0" (eax), "1" (p1));
  return ret;
}

static int syscall2(int function, int p1, int p2)
{
  int eax = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  asm volatile("int $255" : "=a" (ret), "=b" (errno) : "0" (eax), "1" (p1), "c" (p2));
  return ret;
}

static int syscall3(int function, int p1, int p2, int p3)
{
  int eax = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  asm volatile("int $255" : "=a" (ret), "=b" (errno) : "0" (eax), "1" (p1), "c" (p2), "d" (p3));
  return ret;
}

static int syscall4(int function, int p1, int p2, int p3, int p4)
{
  int eax = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  asm volatile("int $255" : "=a" (ret), "=b" (errno) : "0" (eax), "1" (p1), "c" (p2), "d" (p3), "S" (p4));
  return ret;
}

static int syscall5(int function, int p1, int p2, int p3, int p4, int p5)
{
  int eax = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  asm volatile("int $255" : "=a" (ret), "=b" (errno) : "0" (eax), "1" (p1), "c" (p2), "d" (p3), "S" (p4), "D" (p5));
  return ret;
}
