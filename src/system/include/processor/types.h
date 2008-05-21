/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

#if defined(X86)
  #include <processor/x86/types.h>
  #define PROCESSOR_SPECIFIC_NAME(x) X86##x
#elif defined(X64)
  #include <processor/x64/types.h>
  #define PROCESSOR_SPECIFIC_NAME(x) X64##x
#elif defined(MIPS32)
  #include <processor/mips32/types.h>
  #define PROCESSOR_SPECIFIC_NAME(x) MIPS32##x
#elif defined(MIPS64)
  #include <processor/mips64/types.h>
  #define PROCESSOR_SPECIFIC_NAME(x) MIPS64##x
#elif defined(ARM_COMMON)
  #include <processor/arm_common/types.h>
  #define PROCESSOR_SPECIFIC_NAME(x) ARM##x
#elif defined(PPC32)
  #include <processor/ppc32/types.h>
  #define PROCESSOR_SPECIFIC_NAME(x) PPC32##x
#endif

// NOTE: This throws a compile-time error if this header is not adapted for
//       the selected processor architecture
#if !defined(PROCESSOR_SPECIFIC_NAME)
  #error Unknown processor architecture
#endif

/** @addtogroup kernelprocessor
 * @{ */

// NOTE: If a newly added processor architecture does not supply all the
//       needed types, you will get an error here

/** Define an 8bit signed integer type */
typedef PROCESSOR_SPECIFIC_NAME(int8_t) int8_t;
/** Define an 8bit unsigned integer type */
typedef PROCESSOR_SPECIFIC_NAME(uint8_t) uint8_t;
/** Define an 16bit signed integer type */
typedef PROCESSOR_SPECIFIC_NAME(int16_t) int16_t;
/** Define an 16bit unsigned integer type */
typedef PROCESSOR_SPECIFIC_NAME(uint16_t) uint16_t;
/** Define a 32bit signed integer type */
typedef PROCESSOR_SPECIFIC_NAME(int32_t) int32_t;
/** Define a 32bit unsigned integer type */
typedef PROCESSOR_SPECIFIC_NAME(uint32_t) uint32_t;
/** Define a 64bit signed integer type */
typedef PROCESSOR_SPECIFIC_NAME(int64_t) int64_t;
/** Define a 64bit unsigned integer type */
typedef PROCESSOR_SPECIFIC_NAME(uint64_t) uint64_t;

/** Define a signed integer type for pointer arithmetic */
typedef PROCESSOR_SPECIFIC_NAME(intptr_t) intptr_t;
/** Define an unsigned integer type for pointer arithmetic */
typedef PROCESSOR_SPECIFIC_NAME(uintptr_t) uintptr_t;

/** Define a unsigned integer type for physical pointer arithmetic */
typedef PROCESSOR_SPECIFIC_NAME(physical_uintptr_t) physical_uintptr_t;

/** Define an unsigned integer type for processor registers */
typedef PROCESSOR_SPECIFIC_NAME(processor_register_t) processor_register_t;

/** Define ssize_t */
typedef PROCESSOR_SPECIFIC_NAME(ssize_t) ssize_t;
/** Define size_t */
typedef PROCESSOR_SPECIFIC_NAME(size_t) size_t;

// NOTE: This should be defined in the file included at the top of this file
//       if this processor architecture does not support I/O ports
#if !defined(KERNEL_PROCESSOR_NO_PORT_IO)
  /** Define an I/O port type */
  typedef PROCESSOR_SPECIFIC_NAME(io_port_t) io_port_t;
#endif

#if !defined(PAGE_SIZE)
  #error PAGE_SIZE not defined
#endif

/** @} */

#undef PROCESSOR_SPECIFIC_NAME

#endif
