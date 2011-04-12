/*
 * Copyright (c) 2011 Matthew Iselin
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
#ifndef _MEMLOG_H
#define _MEMLOG_H


#ifdef MEMORY_LOGGING_ENABLED

#ifndef _MACHINE_H
#define MACHINE_FORWARD_DECL_ONLY
#include <machine/Machine.h>
#include <machine/Serial.h>
#endif

#define DUMP_MEM_INFO  \
do { \
    extern size_t g_FreePages; \
    extern size_t g_AllocedPages; \
    Serial *pSerial = Machine::instance().getSerial(1); \
    NormalStaticString str; \
    str += "Heap: "; \
    str += (reinterpret_cast<uintptr_t>(VirtualAddressSpace::getKernelAddressSpace().m_HeapEnd)-0xC0000000) / 1024; \
    str += "K\tPages: "; \
    str += (g_AllocedPages * 4096) / 1024; \
    str += "K\t Free: "; \
    str += (g_FreePages * 4096) / 1024; \
    str += "K\n"; \
    pSerial->write(str); \
} while(0)

#else

#define DUMP_MEM_INFO

#endif

#endif
