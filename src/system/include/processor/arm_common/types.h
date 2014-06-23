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

#ifndef KERNEL_PROCESSOR_ARM_TYPES_H
#define KERNEL_PROCESSOR_ARM_TYPES_H

/** @addtogroup kernelprocessorarm
 * @{ */

/** Define an 8bit signed integer type */
typedef signed char ARMint8_t;
/** Define an 8bit unsigned integer type */
typedef unsigned char ARMuint8_t;
/** Define an 16bit signed integer type */
typedef signed short ARMint16_t;
/** Define an 16bit unsigned integer type */
typedef unsigned short ARMuint16_t;
/** Define a 32bit signed integer type */
typedef signed long ARMint32_t;
/** Define a 32bit unsigned integer type */
typedef unsigned long ARMuint32_t;
/** Define a 64bit signed integer type */
typedef signed long long ARMint64_t;
/** Define a 64bit unsigned integer type */
typedef unsigned long long ARMuint64_t;

/** Define a signed integer type for pointer arithmetic */
typedef ARMint32_t ARMintptr_t;
/** Define an unsigned integer type for pointer arithmetic */
typedef ARMuint32_t ARMuintptr_t;

/** Define a unsigned integer type for physical pointer arithmetic */
typedef ARMuint32_t ARMphysical_uintptr_t;

/** Define an unsigned integer type for the processor registers */
typedef ARMuint32_t ARMprocessor_register_t;

/** Define ssize_t */
typedef ARMint32_t ARMssize_t;
/** Define size_t */
typedef ARMuint32_t ARMsize_t;

/** Define an I/O port type */
typedef ARMuint16_t ARMio_port_t;

/** No I/O port type */
#define KERNEL_PROCESSOR_NO_PORT_IO 1

/** Define the size of one physical page */
#define PAGE_SIZE 4096

/** Clean up preprocessor namespace to force use of our types. */
#undef __INTPTR_TYPE__
#undef __UINTPTR_TYPE__

/** @} */

#endif
