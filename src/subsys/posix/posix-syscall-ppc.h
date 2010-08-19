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

/// \todo add errno.
static int syscall0(int function)
{
  register unsigned int r3 __asm__ ("r3") = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  asm volatile("sc" : "=r" (r3) : "r" (r3));
  return r3;
}

static int syscall1(int function, int p1)
{
  register unsigned int r3 __asm__ ("r3") = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  register unsigned int r4 __asm__ ("r6") = p1;
  asm volatile("sc" : "=r" (r3) : "r" (r3), "r"(r4));
  return r3;
}

static int syscall2(int function, int p1, int p2)
{
  register unsigned int r3 __asm__ ("r3") = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  register unsigned int r4 __asm__ ("r6") = p1;
  register unsigned int r5 __asm__ ("r7") = p2;
  asm volatile("sc" : "=r" (r3) : "r" (r3), "r" (r4), "r" (r5));
  return r3;
}

static int syscall3(int function, int p1, int p2, int p3)
{
  register unsigned int r3 __asm__ ("r3") = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  register unsigned int r4 __asm__ ("r6") = p1;
  register unsigned int r5 __asm__ ("r7") = p2;
  register unsigned int r6 __asm__ ("r8") = p3;
  asm volatile("sc" : "=r" (r3) : "r" (r3), "r" (r4), "r" (r5), "r" (r6));
  return r3;
}

static int syscall4(int function, int p1, int p2, int p3, int p4)
{
  register unsigned int r3 __asm__ ("r3") = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  register unsigned int r4 __asm__ ("r6") = p1;
  register unsigned int r5 __asm__ ("r7") = p2;
  register unsigned int r6 __asm__ ("r8") = p3;
  register unsigned int r7 __asm__ ("r9") = p4;
  asm volatile("sc" : "=r" (r3) : "r" (r3), "r" (r4), "r" (r5), "r" (r6), "r"(r7));
  return r3;
}

static int syscall5(int function, int p1, int p2, int p3, int p4, int p5)
{
  register unsigned int r3 __asm__ ("r3") = ((POSIX_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  register unsigned int r4 __asm__ ("r6") = p1;
  register unsigned int r5 __asm__ ("r7") = p2;
  register unsigned int r6 __asm__ ("r8") = p3;
  register unsigned int r7 __asm__ ("r9") = p4;
  register unsigned int r8 __asm__ ("r10") = p5;
  asm volatile("sc" : "=r" (r3) : "r" (r3), "r" (r4), "r" (r5), "r" (r6), "r"(r7), "r" (r8));
  return r3;
}
