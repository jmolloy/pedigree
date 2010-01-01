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

#include <machine/Machine.h>
#include <utilities/StaticString.h>

extern "C" void __cyg_profile_func_enter (void *func_address, void *call_site)
  __attribute__((no_instrument_function));
extern "C" void __cyg_profile_func_exit (void *func_address, void *call_site)
  __attribute__((no_instrument_function));

#ifdef X86_COMMON
#define OUT(c) asm volatile ("outb %%al, %%dx" :: "d"(0x2f8), "a"(c))
#else
#define OUT(C)
#endif

extern "C" void __cyg_profile_func_enter (void *func_address, void *call_site)
{
    uint32_t _func_address = reinterpret_cast<uintptr_t>(func_address)&0xFFFFFFFF;
    uint32_t _call_site = reinterpret_cast<uintptr_t>(call_site)&0xFFFFFFFF;

//    register int eflags;
//    asm volatile ("pushf; pop %0; cli" : "=r" (eflags));
    OUT('E');
    OUT('0' + ((_func_address>>(32-4))&0xF));
    OUT('0' + ((_func_address>>(32-8))&0xF));
    OUT('0' + ((_func_address>>(32-12))&0xF));
    OUT('0' + ((_func_address>>(32-16))&0xF));
    OUT('0' + ((_func_address>>(32-20))&0xF));
    OUT('0' + ((_func_address>>(32-24))&0xF));
    OUT('0' + ((_func_address>>(32-28))&0xF));
    OUT('0' + ((_func_address>>(32-32))&0xF));
    OUT(' ');
    OUT('0' + ((_call_site>>(32-4))&0xF));
    OUT('0' + ((_call_site>>(32-8))&0xF));
    OUT('0' + ((_call_site>>(32-12))&0xF));
    OUT('0' + ((_call_site>>(32-16))&0xF));
    OUT('0' + ((_call_site>>(32-20))&0xF));
    OUT('0' + ((_call_site>>(32-24))&0xF));
    OUT('0' + ((_call_site>>(32-28))&0xF));
    OUT('0' + ((_call_site>>(32-32))&0xF));
    OUT('\n');
//    asm volatile("push %0; popf" ::"r" (eflags));
}

extern "C" void __cyg_profile_func_exit (void *func_address, void *call_site)
{
}
