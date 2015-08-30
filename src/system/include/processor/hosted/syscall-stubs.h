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

#ifndef SERVICE
#error syscall-stubs.h requires SERVICE to be defined
#endif
#ifndef SERVICE_ERROR
#error syscall-stubs.h requires SERVICE_ERROR to be defined
#endif
#ifndef SERVICE_INIT
#error syscall-stubs.h requires SERVICE_INIT to be defined
#endif

static long syscall0(long function)
{
  SERVICE_INIT;
  __builtin_trap();
  return 0;
}

static long syscall1(long function, long p1)
{
  SERVICE_INIT;
  __builtin_trap();
  return 0;
}

static long syscall2(long function, long p1, long p2)
{
  SERVICE_INIT;
  __builtin_trap();
  return 0;
}

static long syscall3(long function, long p1, long p2, long p3)
{
  SERVICE_INIT;
  __builtin_trap();
  return 0;
}

static long syscall4(long function, long p1, long p2, long p3, long p4)
{
  SERVICE_INIT;
  __builtin_trap();
  return 0;
}

static long syscall5(long function, long p1, long p2, long p3, long p4, long p5)
{
  SERVICE_INIT;
  __builtin_trap();
  return 0;
}
