/*
 * Copyright (c) 2008 James Molloy, James Pritchett, JÃ¶rg PfÃ¤hler, Matthew Iselin
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
#ifndef KERNEL_PROCESSOR_MIPS64_TYPES_H
#define KERNEL_PROCESSOR_MIPS64_TYPES_H

/** @addtogroup kernelprocessorMIPS64 MIPS64
 * MIPS64 processor-specific kernel
 *  @ingroup kernelprocessor
 * @{ */

/** Define an 8bit signed integer type */
typedef signed char MIPS64int8_t;
/** Define an 8bit unsigned integer type */
typedef unsigned char MIPS64uint8_t;
/** Define an 16bit signed integer type */
typedef signed short MIPS64int16_t;
/** Define an 16bit unsigned integer type */
typedef unsigned short MIPS64uint16_t;
/** Define a 32bit signed integer type */
typedef signed int MIPS64int32_t;
/** Define a 32bit unsigned integer type */
typedef unsigned int MIPS64uint32_t;
/** Define a 64bit signed integer type */
typedef signed long MIPS64int64_t;
/** Define a 64bit unsigned integer type */
typedef unsigned long MIPS64uint64_t;

/** Define a signed integer type for pointer arithmetic */
typedef MIPS64int64_t MIPS64intptr_t;
/** Define an unsigned integer type for pointer arithmetic */
typedef MIPS64uint64_t MIPS64uintptr_t;

/** Define an unsigned integer type for the processor registers */
typedef MIPS64uint64_t MIPS64processor_register_t;

/** Define ssize_t */
typedef MIPS64int32_t MIPS64ssize_t;
/** Define size_t */
typedef MIPS64uint32_t MIPS64size_t;

// No I/O port type
#define KERNEL_PROCESSOR_NO_PORT_IO

/** @} */

#endif
