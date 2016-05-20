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

#ifndef KERNEL_MEMORYTRACING_H
#define KERNEL_MEMORYTRACING_H

#ifdef MEMORY_TRACING
#include <utilities/StaticString.h>

namespace MemoryTracing
{
    enum AllocationTrace {
        Allocation = 'A',
        Free = 'F',
        Metadata = 'M',
        PageAlloc = 'P',
        PageFree = 'X',
    };

    const int num_backtrace_entries = 5;

    union AllocationTraceEntry
    {
        struct
        {
            AllocationTrace type;
            uint32_t sz;
            uint32_t ptr;
            uint32_t bt[num_backtrace_entries];
        } data;

        char buf[sizeof(data)];
    };
}

/**
 * Adds an allocation field to the memory trace.
 *
 * This includes a full backtrace which will NOT include the caller of
 * traceAllocation.
 */
extern void traceAllocation(void *ptr, MemoryTracing::AllocationTrace type, size_t size);

/**
 * Adds a metadata field to the memory trace.
 *
 * This is typically used to define the region in which a module has been
 * loaded, so the correct debug symbols can be loaded and used.
 */
extern void traceMetadata(NormalStaticString str, void *p1, void *p2);
#endif

#endif
