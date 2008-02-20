/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#ifndef KERNEL_PROCESSOR_X64_TYPES_H
#define KERNEL_PROCESSOR_X64_TYPES_H

/** @addtogroup kernelprocessorx64 x64
 * x64 processor-specific kernel
 *  @ingroup kernelprocessor
 * @{ */

/** Define an 8bit signed integer type */
typedef signed char X64int8_t;
/** Define an 8bit unsigned integer type */
typedef unsigned char X64uint8_t;
/** Define an 16bit signed integer type */
typedef signed short X64int16_t;
/** Define an 16bit unsigned integer type */
typedef unsigned short X64uint16_t;
/** Define a 32bit signed integer type */
typedef signed int X64int32_t;
/** Define a 32bit unsigned integer type */
typedef unsigned int X64uint32_t;
/** Define a 64bit signed integer type */
typedef signed long X64int64_t;
/** Define a 64bit unsigned integer type */
typedef unsigned long X64uint64_t;

/** Define a signed integer type for pointer arithmetic */
typedef X64int64_t X64intptr_t;
/** Define an unsigned integer type for pointer arithmetic */
typedef X64uint64_t X64uintptr_t;

/** Define ssize_t */
typedef X64int64_t X64ssize_t;
/** Define size_t */
typedef X64uint64_t X64size_t;

/** Define an I/O port type */
typedef X64uint16_t X64io_port_t;

/** @} */

#endif
