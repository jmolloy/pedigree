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

#ifndef KERNEL_ASSERT_H
#define KERNEL_ASSERT_H

/** @addtogroup kernel
 * @{ */

/** If the passed argument resolves to a Boolean false value, execution will be halted
 *  and a message displayed.
 */
#ifdef DEBUGGER
#define assert(x) _assert(x, __FILE__, __LINE__, __PRETTY_FUNCTION__)

#ifndef USE_DEBUG_ALLOCATOR
#define assert_heap_ptr_valid(x) _assert(_assert_ptr_valid(reinterpret_cast<uintptr_t>(x)), __FILE__, __LINE__, __PRETTY_FUNCTION__)
#else
#define assert_heap_ptr_valid
#endif

#else
#define assert
#define assert_heap_ptr_valid
#endif

#ifndef __cplusplus
#define bool char
#endif

/// Internal use only, the assert() macro passes the additional arguments automatically
#ifdef __cplusplus
extern "C" {
#endif

void _assert(bool b, const char *file, int line, const char *func);
bool _assert_ptr_valid(uintptr_t x);

#ifdef __cplusplus
};
#endif

/** @} */

#endif
