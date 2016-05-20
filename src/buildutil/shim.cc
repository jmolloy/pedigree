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

#define PEDIGREE_EXTERNAL_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>

#include <time/Time.h>

void *g_pBootstrapInfo = 0;

namespace Time
{

Timestamp getTime(bool sync)
{
    return time(NULL);
}

}  // Time

extern "C"
void panic(const char *s)
{
    fprintf(stderr, "PANIC: %s\n", s);
    abort();
}

namespace SlamSupport
{
uintptr_t getHeapBase()
{
    return 0x10000000ULL;
}

uintptr_t getHeapEnd()
{
    return 0x50000000ULL;
}

void getPageAt(void *addr)
{
    void *r = mmap(addr, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE |
        MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    if (r == MAP_FAILED)
    {
        fprintf(stderr, "map failed: %s\n", strerror(errno));
        abort();
    }
}

void unmapPage(void *page)
{
    munmap(page, 0x1000);
}

void unmapAll()
{
    munmap((void *) getHeapBase(), getHeapEnd() - getHeapBase());
}
}  // namespace SlamSupport
