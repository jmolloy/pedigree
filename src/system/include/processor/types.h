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
#ifndef KERNEL_PROCESSOR_TYPES_H
#define KERNEL_PROCESSOR_TYPES_H

#ifdef X86
  #include <processor/x86/types.h>
  #define PROCESSOR_NAMESPACE x86
#endif
#ifdef X86_64
  #include <processor/x64/types.h>
  #define PROCESSOR_NAMESPACE x64
#endif


// NOTE: This throws a compile-time error if this header is not adapted for
//       the selected processor architecture
#ifndef PROCESSOR_NAMESPACE
  #error Unknown processor architecture
#endif


#ifdef __cplusplus

  /** @addtogroup kernelprocessor processor-specifc kernel
   * processor-specific kernel interface
   *  @ingroup kernel
   * @{ */

  // NOTE: If a newly added processor architecture does not supply all the
  //       needed types, you will get an error here

  /** Define an 8bit signed integer type */
  typedef PROCESSOR_NAMESPACE::int8_t int8_t;
  /** Define an 8bit unsigned integer type */
  typedef PROCESSOR_NAMESPACE::uint8_t uint8_t;
  /** Define an 16bit signed integer type */
  typedef PROCESSOR_NAMESPACE::int16_t int16_t;
  /** Define an 16bit unsigned integer type */
  typedef PROCESSOR_NAMESPACE::uint16_t uint16_t;
  /** Define a 32bit signed integer type */
  typedef PROCESSOR_NAMESPACE::int32_t int32_t;
  /** Define a 32bit unsigned integer type */
  typedef PROCESSOR_NAMESPACE::uint32_t uint32_t;

  // NOTE: This should be defined in the file included at the top of this file
  //       if this processor architecture does not support a 64bit data type
  #ifndef KERNEL_PROCESSOR_NO_64BIT_TYPE
    /** Define a 64bit signed integer type */
    typedef PROCESSOR_NAMESPACE::int64_t int64_t;
    /** Define a 64bit unsigned integer type */
    typedef PROCESSOR_NAMESPACE::uint64_t uint64_t;
  #endif

  /** Define a signed integer type for pointer arithmetic */
  typedef PROCESSOR_NAMESPACE::intptr_t intptr_t;
  /** Define an unsigned integer type for pointer arithmetic */
  typedef PROCESSOR_NAMESPACE::uintptr_t uintptr_t;

  /** Define ssize_t */
  typedef PROCESSOR_NAMESPACE::ssize_t ssize_t;
  /** Define size_t */
  typedef PROCESSOR_NAMESPACE::size_t size_t;

  /** Define a type that can hold the value of a processor register */
  typedef PROCESSOR_NAMESPACE::processor_register_t processor_register_t;

  // NOTE: This should be defined in the file included at the top of this file
  //       if this processor architecture does not support I/O ports
  #ifndef KERNEL_PROCESSOR_NO_PORT_IO
    /** Define an I/O port type */
    typedef PROCESSOR_NAMESPACE::io_port_t io_port_t;
  #endif

#endif

/** @} */

#undef PROCESSOR_NAMESPACE

#endif
