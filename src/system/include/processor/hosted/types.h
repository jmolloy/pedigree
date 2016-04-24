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

#ifndef KERNEL_PROCESSOR_HOSTED_TYPES_H
#define KERNEL_PROCESSOR_HOSTED_TYPES_H

/** @addtogroup kernelprocessorhosted
 * @{ */

#include <stdint.h>
#include <stddef.h>

#define HOSTED_TYPE(x) typedef x HOSTED##x;

/** Basic integral types. */
HOSTED_TYPE(int8_t);
HOSTED_TYPE(uint8_t);
HOSTED_TYPE(int16_t);
HOSTED_TYPE(uint16_t);
HOSTED_TYPE(int32_t);
HOSTED_TYPE(uint32_t);
HOSTED_TYPE(int64_t);
HOSTED_TYPE(uint64_t);

/** Integral pointer types. */
HOSTED_TYPE(intptr_t);
HOSTED_TYPE(uintptr_t);

/** Pedigree-specific types. */
typedef HOSTEDuintptr_t HOSTEDphysical_uintptr_t;
typedef HOSTEDuintptr_t HOSTEDprocessor_register_t;

/** Size types. */
typedef unsigned long HOSTEDsize_t;
typedef long HOSTEDssize_t;

/** Port I/O. */
typedef HOSTEDuint16_t HOSTEDio_port_t;

/** Define the size of one physical page */
#define PAGE_SIZE 4096

/** No port I/O on hosted systems. */
#define KERNEL_PROCESSOR_NO_PORT_IO 1

/** @} */

#endif
