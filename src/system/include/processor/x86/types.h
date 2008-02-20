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
#ifndef KERNEL_PROCESSOR_X86_TYPES_H
#define KERNEL_PROCESSOR_X86_TYPES_H

/** @addtogroup kernelprocessorx86 x86
 * x86 processor-specific kernel
 *  @ingroup kernelprocessor
 * @{ */

/** Define an 8bit signed integer type */
typedef signed char X86int8_t;
/** Define an 8bit unsigned integer type */
typedef unsigned char X86uint8_t;
/** Define an 16bit signed integer type */
typedef signed short X86int16_t;
/** Define an 16bit unsigned integer type */
typedef unsigned short X86uint16_t;
/** Define a 32bit signed integer type */
typedef signed int X86int32_t;
/** Define a 32bit unsigned integer type */
typedef unsigned int X86uint32_t;
/** Define a 64bit signed integer type */
typedef signed long long X86int64_t;
/** Define a 64bit unsigned integer type */
typedef unsigned long long X86uint64_t;

/** Define a signed integer type for pointer arithmetic */
typedef X86int32_t X86intptr_t;
/** Define an unsigned integer type for pointer arithmetic */
typedef X86uint32_t X86uintptr_t;

/** Define ssize_t */
typedef signed long X86ssize_t;
/** Define size_t */
typedef unsigned long X86size_t;

/** Define an I/O port type */
typedef X86uint16_t X86io_port_t;

/** @} */

#endif
