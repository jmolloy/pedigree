/*
 * 
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

// If USE_PIC_SYSCALLS is defined, the EBX register will be preserved, as GCC
// refuses to accept it as a clobbered register in inline assembly with -fPIC.

static int syscall0(int function)
{
  int eax = ((SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  SERVICE_INIT;
#ifdef USE_PIC_SYSCALLS
  asm volatile("push %%ebx; \
                mov %1, %%ebx; \
                int $255; \
                mov %%ebx, %1; \
                pop %%ebx" : "=a" (ret), "=r" (SERVICE_ERROR) : "0" (eax));
#else
  asm volatile("int $255" : "=a" (ret), "=b" (SERVICE_ERROR) : "0" (eax));
#endif
  return ret;
}

static int syscall1(int function, int p1)
{
  int eax = ((SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  SERVICE_INIT;
#ifdef USE_PIC_SYSCALLS
  asm volatile("push %%ebx; \
                mov %1, %%ebx; \
                int $255; \
                mov %%ebx, %1; \
                pop %%ebx" : "=a" (ret), "=r" (SERVICE_ERROR) : "0" (eax), "1" (p1));
#else
  asm volatile("int $255" : "=a" (ret), "=b" (SERVICE_ERROR) : "0" (eax), "1" (p1));
#endif
  return ret;
}

static int syscall2(int function, int p1, int p2)
{
  int eax = ((SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  SERVICE_INIT;
#ifdef USE_PIC_SYSCALLS
  asm volatile("push %%ebx; \
                mov %1, %%ebx; \
                int $255; \
                mov %%ebx, %1; \
                pop %%ebx" : "=a" (ret), "=r" (SERVICE_ERROR) : "0" (eax), "1" (p1), "c" (p2));
#else
  asm volatile("int $255" : "=a" (ret), "=b" (SERVICE_ERROR) : "0" (eax), "1" (p1), "c" (p2));
#endif
  return ret;
}

static int syscall3(int function, int p1, int p2, int p3)
{
  int eax = ((SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  SERVICE_INIT;
#ifdef USE_PIC_SYSCALLS
  asm volatile("push %%ebx; \
                mov %1, %%ebx; \
                int $255; \
                mov %%ebx, %1; \
                pop %%ebx" : "=a" (ret), "=r" (SERVICE_ERROR) : "0" (eax), "1" (p1), "c" (p2), "d" (p3));
#else
  asm volatile("int $255" : "=a" (ret), "=b" (SERVICE_ERROR) : "0" (eax), "1" (p1), "c" (p2), "d" (p3));
#endif
  return ret;
}

static int syscall4(int function, int p1, int p2, int p3, int p4)
{
  int eax = ((SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  SERVICE_INIT;
#ifdef USE_PIC_SYSCALLS
  asm volatile("push %%ebx; \
                mov %1, %%ebx; \
                int $255; \
                mov %%ebx, %1; \
                pop %%ebx" : "=a" (ret), "=r" (SERVICE_ERROR) : "0" (eax), "1" (p1), "c" (p2), "d" (p3), "S" (p4));
#else
  asm volatile("int $255" : "=a" (ret), "=b" (SERVICE_ERROR) : "0" (eax), "1" (p1), "c" (p2), "d" (p3), "S" (p4));
#endif
  return ret;
}

static int syscall5(int function, int p1, int p2, int p3, int p4, int p5)
{
  int eax = ((SERVICE&0xFFFF) << 16) | (function&0xFFFF);
  int ret;
  SERVICE_INIT;
#ifdef USE_PIC_SYSCALLS
  asm volatile("push %%ebx; \
                mov %1, %%ebx; \
                int $255; \
                mov %%ebx, %1; \
                pop %%ebx" : "=a" (ret), "+m" (p1) : "0" (eax), "c" (p2), "d" (p3), "S" (p4), "D" (p5));
  SERVICE_ERROR = p1;
#else
  asm volatile("int $255" : "=a" (ret), "=b" (SERVICE_ERROR) : "0" (eax), "1" (p1), "c" (p2), "d" (p3), "S" (p4), "D" (p5));
#endif
  return ret;
}
