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

#include <machine/Machine.h>
#include <utilities/StaticString.h>
#include "instrument.h"

#define USE_LITE_RECORD     1

extern "C"
{
void __cyg_profile_func_enter (void *func_address, void *call_site)
  __attribute__((no_instrument_function)) __attribute__((hot));

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

static volatile int g_WrittenFirst = 0;

void __cyg_profile_func_enter (void *func_address, void *call_site)
{
#ifdef INSTRUMENTATION
    // NOTE: you cannot call anything in this function, as doing so would
    // re-enter. That means hand-crafted serial writes are necessary.
#ifdef X64
    if (UNLIKELY(g_WrittenFirst == 0))
    {
        if (__sync_bool_compare_and_swap(&g_WrittenFirst, 0, 1))
        {
            uint8_t flag = 0;
#if USE_LITE_RECORD
            flag |= INSTRUMENT_GLOBAL_LITE;
#endif
            asm volatile("outb %%al, %%dx" :: "d" (COM2), "a" (flag));
        }
    }

#if USE_LITE_RECORD
    LiteInstrumentationRecord record;
#else
    InstrumentationRecord record;
    record.data.flags = INSTRUMENT_RECORD_ENTRY;
    record.data.caller = reinterpret_cast<uintptr_t>(call_site);
    record.data.magic = INSTRUMENT_MAGIC;
#endif
    record.data.function = reinterpret_cast<uintptr_t>(func_address);

    // Semi-unrolled, all-in-one assembly serial-port write.
    uintptr_t x = 0;
    size_t y = 0;
    asm volatile("pushf; cli;" \
        ".1:\n" \
        "movq (%0), %%rax;" \
        "outb %%al, %%dx;" \
        "shr $8, %%rax;" \
        "outb %%al, %%dx;" \
        "shr $8, %%rax;" \
        "outb %%al, %%dx;" \
        "shr $8, %%rax;" \
        "outb %%al, %%dx;" \
        "add $4, %0;" \
        "sub $4, %1;" \
        "jnz .1; popf" : "=&r" (x), "=&r" (y) : "d" (COM2), "0" (record.buffer),
            "1" (sizeof record.buffer) : "rax", "memory");
#endif
#endif
}

}  // extern "C"
