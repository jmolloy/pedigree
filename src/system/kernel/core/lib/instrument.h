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

#ifndef KERNEL_INSTRUMENT_H
#define KERNEL_INSTRUMENT_H

#include <compiler.h>
#include <processor/types.h>

// Global flags are held within the first byte written to the instrumentation
// stream, and control things like which data types to use.
#define INSTRUMENT_GLOBAL_LITE     (1 << 0)

// Record flags define how to interpret the specific records.
#define INSTRUMENT_RECORD_ENTRY    (1 << 0)
#define INSTRUMENT_RECORD_SHORTPTR (1 << 4)

#define INSTRUMENT_MAGIC 0x1090U

/**
 * InstrumentationRecord is the full-size, full-featured instrumentation type.
 * It provides information about callers and allows for flexibility via flags.
 */
typedef union
{
    struct
    {
        uint8_t flags;
        uintptr_t function;
        uintptr_t caller;
        uint16_t magic;
    } data;

    uint8_t buffer[sizeof(data)];
} InstrumentationRecord;

/**
 * LiteInstrumentationRecord only writes each entered function's address to
 * the instrumentation stream, which can be a relatively significant
 * improvement in performance.
 *
 * This is done at the cost of flexibility (because there's no flags), and at
 * the cost of integrity (because there's no magic number for verification).
 */
typedef union
{
    struct
    {
        /**
         * The kernel is always going to have 32 one bits as the first part of
         * the code addresses it works with, including module code. So, we can
         * write even less data (improving performance) for lite records by
         * simply writing the lower 32 bits of the address.
         */
        uint32_t function;
    } data;

    uint8_t buffer[sizeof(data)];

    /// For SFINAE expansion in data processing.
    typedef uintptr_t lite;
} LiteInstrumentationRecord;

#endif  // KERNEL_INSTRUMENT_H
