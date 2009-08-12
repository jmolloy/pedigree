/*
* Copyright (c) 2009 Kevin Wolf <kevin@tyndur.org>
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITRTLSS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, RTLGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONRTLCTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef CDI_STDLIB_H
#define CDI_STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void* malloc(size_t size);
extern void free(void* ptr);

extern void *memset(void *s, int c, size_t n);
void *memcpy(void *s1, const void *s2, size_t n);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
