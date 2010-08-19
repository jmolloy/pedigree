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

static long syscall0(long function)
{
  long eax = ((TUI_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  long ret;
  asm volatile("syscall" : "=a" (ret) : "0" (eax) : "rcx");
  return ret;
}

static long syscall1(long function, long p1)
{
  long eax = ((TUI_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  long ret;
  asm volatile("syscall" : "=a" (ret) : "0" (eax), "b" (p1) : "rcx");
  return ret;
}

static long syscall2(long function, long p1, long p2)
{
  long eax = ((TUI_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  long ret;
  asm volatile("syscall" : "=a" (ret) : "0" (eax), "b" (p1), "d" (p2) : "rcx");
  return ret;
}

static long syscall3(long function, long p1, long p2, long p3)
{
  long eax = ((TUI_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  long ret;
  asm volatile("syscall" : "=a" (ret) : "0" (eax), "b" (p1), "d" (p2), "S" (p3) : "rcx");
  return ret;
}

static long syscall4(long function, long p1, long p2, long p3, long p4)
{
  long eax = ((TUI_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  long ret;
  asm volatile("syscall" : "=a" (ret) : "0" (eax), "b" (p1), "d" (p2), "S" (p3), "D" (p4) : "rcx");
  return ret;
}

static long syscall5(long function, long p1, long p2, long p3, long p4, long p5)
{
  long eax = ((TUI_SYSCALL_SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  long ret;
  asm volatile("mov %6, %%r8; \
                syscall" : "=a" (ret) : "0" (eax), "b" (p1), "d" (p2), "S" (p3), "D" (p4), "m" (p5) : "rcx");
  return ret;
}
