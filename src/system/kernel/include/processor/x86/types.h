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

#ifdef __cplusplus
namespace x86
{
#endif

  /** @addtogroup kernelprocessorx86 x86
   * x86 processor-specific kernel
   *  @ingroup kernelprocessor
   * @{ */

  /** Define an 8bit signed integer type */
  typedef signed char int8_t;
  /** Define an 8bit unsigned integer type */
  typedef unsigned char uint8_t;
  /** Define an 16bit signed integer type */
  typedef signed short int16_t;
  /** Define an 16bit unsigned integer type */
  typedef unsigned short uint16_t;
  /** Define a 32bit signed integer type */
  typedef signed int int32_t;
  /** Define a 32bit unsigned integer type */
  typedef unsigned int uint32_t;
  /** Define a 64bit signed integer type */
  typedef signed long long int64_t;
  /** Define a 64bit unsigned integer type */
  typedef unsigned long long uint64_t;

  /** Define a signed integer type for pointer arithmetic */
  typedef int32_t intptr_t;
  /** Define an unsigned integer type for pointer arithmetic */
  typedef uint32_t uintptr_t;

  /** Define ssize_t */
  typedef signed long ssize_t;
  /** Define size_t */
  typedef unsigned long size_t;

  /** Define a type that can hold the value of a processor register */
  typedef uint32_t processor_register_t;

  /** Define an I/O port type */
  typedef uint16_t io_port_t;

  /** @} */

#ifdef __cplusplus
}
#endif

#endif
