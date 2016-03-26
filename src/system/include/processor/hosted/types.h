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

/** Define an 8bit signed integer type */
typedef signed char HOSTEDint8_t;
/** Define an 8bit unsigned integer type */
typedef unsigned char HOSTEDuint8_t;
/** Define an 16bit signed integer type */
typedef signed short HOSTEDint16_t;
/** Define an 16bit unsigned integer type */
typedef unsigned short HOSTEDuint16_t;
/** Define a 32bit signed integer type */
typedef signed int HOSTEDint32_t;
/** Define a 32bit unsigned integer type */
typedef unsigned int HOSTEDuint32_t;
/** Define a 64bit signed integer type */
#ifdef INT64_MAX
typedef int64_t HOSTEDint64_t;
#else
typedef signed long HOSTEDint64_t;
#endif
/** Define a 64bit unsigned integer type */
#ifdef UINT64_MAX
typedef uint64_t HOSTEDuint64_t;
#else
typedef unsigned long HOSTEDuint64_t;
#endif

/** Define a signed integer type for pointer arithmetic */
#ifdef INTPTR_MAX
typedef intptr_t HOSTEDintptr_t;
#else
typedef HOSTEDint64_t HOSTEDintptr_t;
#endif
/** Define an unsigned integer type for pointer arithmetic */
#ifdef UINTPTR_MAX
typedef uintptr_t HOSTEDuintptr_t;
#else
typedef HOSTEDuint64_t HOSTEDuintptr_t;
#endif

/** Define a unsigned integer type for physical pointer arithmetic */
typedef HOSTEDuint64_t HOSTEDphysical_uintptr_t;

/** Define an unsigned integer type for the processor registers */
typedef HOSTEDuint64_t HOSTEDprocessor_register_t;

/** Define ssize_t */
#ifdef SIZE_MAX
typedef ssize_t HOSTEDssize_t;
#else
typedef HOSTEDint64_t HOSTEDssize_t;
#endif
/** Define size_t */
#ifdef SIZE_MAX
typedef size_t HOSTEDsize_t;
#else
typedef HOSTEDuint64_t HOSTEDsize_t;
#endif

/** Define an I/O port type */
typedef HOSTEDuint16_t HOSTEDio_port_t;

/** Define the size of one physical page */
#define PAGE_SIZE 4096

/** No port I/O on hosted systems. */
#define KERNEL_PROCESSOR_NO_PORT_IO 1

/** @} */

#endif
