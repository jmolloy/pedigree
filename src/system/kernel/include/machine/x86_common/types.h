/*
 * Copyright (c) 2008 Jörg Pfähler
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
#ifndef KERNEL_MACHINE_X86_COMMON_TYPES_H
#define KERNEL_MACHINE_X86_COMMON_TYPES_H

/** @addtogroup kernelmachinex86common x86-common
 * common-x86 machine-specific kernel
 *  @ingroup kernelmachine
 * @{ */

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
#ifdef X86
  typedef signed long long int64_t;
  typedef unsigned long long uint64_t;
#endif
#ifdef X86_64
  typedef signed long int64_t;
  typedef unsigned long uint64_t;
#endif
typedef signed long ssize_t;
typedef unsigned long size_t;
#ifdef X86
  typedef uint32_t register_t;
#endif
#ifdef X86_64
  typedef uint64_t register_t
#endif

/** Define an I/O port type */
typedef uint16_t IoPort_t;

/** @} */

#endif
