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

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <setjmp.h>

static int i = 0;
static char *p = 0;

static jmp_buf buf;

typedef void (*fn)();

extern void fail();

static void *adjust_pointer(void *p, ssize_t amt)
{
    return (void *) (((uintptr_t) p) + amt);
}

static void sigsegv(int s)
{
    if(i == -1)
    {
        fail();
    }

    switch(i)
    {
        case 0:
            {
                printf("PROT_NONE works, checking read...\n");
                mprotect(p, 0x1000, PROT_READ);
                i = -1;
                char c = *p;
                i = 0;
            }
            break;

        case 1:
            {
                printf("PROT_READ works, checking write...\n");
                mprotect(p, 0x1000, PROT_WRITE);
                i = -1;
                *p = 'Y';
                i = 1;
                mprotect(p, 0x1000, PROT_NONE);
            }
            break;

        case 2:
            {
                printf("PROT_WRITE works, checking exec...\n");
                mprotect(p, 0x1000, PROT_WRITE);
                i = -1;
                *((unsigned char *) p) = 0xC3; // ret
                fn f = (fn) p;
                mprotect(p, 0x1000, PROT_EXEC);
                f();
                i = 2;
            }
            break;

        default:
            printf("Attempting to return to original context...\n");
            mprotect(p, 0x1000, PROT_READ | PROT_WRITE);
            longjmp(buf, 1);
    }

    ++i;
}

static void sigsegv_jumper(int s)
{
    longjmp(buf, 1);
}

static void status(const char *s)
{
    printf(s);
    fflush(stdout);
}

void test_mprotect()
{
    int rc = 0;

    p = mmap(0, 0x10000, PROT_NONE, MAP_PRIVATE | MAP_ANON, 0, 0);

    // Install our custom SIGSEGV handler.
    static struct sigaction act;
    sigprocmask(0, 0, &act.sa_mask);
    act.sa_handler = sigsegv;
#ifdef SA_NODEFER
    act.sa_flags = SA_NODEFER;
#else
    act.sa_flags = 0; // Pedigree doesn't yet have SA_NODEFER (SIGSEGV nests anyway)
#endif
    sigaction(SIGSEGV, &act, 0);

    // Start the test!
    printf("Testing mprotect(2)...\n");
    setjmp(buf);
    *p = 'X';
    printf("mprotect(2) initial test was successful!\n");

    printf("Testing mprotect(2) on ranges of pages...\n");

    i = -1;

    act.sa_handler = sigsegv_jumper;
    sigaction(SIGSEGV, &act, 0);

    // Check for 100% coverage.
    mprotect(p, 0x10000, PROT_WRITE);
    status("Test A... ");
    *p = 'X';
    status("OK\n");
    mprotect(p, 0x10000, PROT_NONE);

    // Check for 100% enclosed.
    mprotect(adjust_pointer(p, -0x1000), 0x12000, PROT_WRITE);
    status("Test B... ");
    if(setjmp(buf) == 1)
        fail();
    *p = 'X';
    p[0x10000 - 1] = 'X';
    status("OK\n");
    mprotect(p, 0x10000, PROT_NONE);

    // Check for overlap at beginning.
    mprotect(adjust_pointer(p, -0x1000), 0x5000, PROT_WRITE);
    status("Test C... ");
    if(setjmp(buf) == 1)
        fail();
    *p = 'X';
    if(setjmp(buf) == 0)
    {
        p[0x5001] = 'X';
        fail();
    }
    status("OK\n");
    mprotect(p, 0x10000, PROT_NONE);

    // Check for overlap at end.
    mprotect(adjust_pointer(p, 0x5000), 0x6000, PROT_WRITE);
    status("Test D... ");
    if(setjmp(buf) == 1)
        fail();
    p[0x5000] = 'X';
    if(setjmp(buf) == 0)
    {
        *p = 'X';
        fail();
    }
    status("OK\n");
    mprotect(p, 0x10000, PROT_NONE);

    // Check middle.
    mprotect(adjust_pointer(p, 0x2000), 0x6000, PROT_WRITE);
    status("Test E... ");
    if(setjmp(buf) == 1)
        fail();
    p[0x2000] = 'X';
    if(setjmp(buf) == 0)
    {
        *p = 'X';
        fail();
    }

    if(setjmp(buf) == 0)
    {
        p[0x8001] = 'X';
        fail();
    }
    status("OK\n");
    mprotect(p, 0x10000, PROT_NONE);

    printf("mprotect(2) page range test was successful!\n");

    // Restore SIGSEGV handler.
    signal(SIGSEGV, SIG_DFL);
}
