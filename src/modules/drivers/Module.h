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

#ifndef MODULE_H
#define MODULE_H

typedef void (*ModuleEntry)();
typedef void (*ModuleExit)();

#define MODULE_NAME(x) const char *g_pModuleName __attribute__((section(".modinfo"))) = x
#define MODULE_ENTRY(x) ModuleEntry g_pModuleEntry __attribute__((section(".modinfo"))) = x
#define MODULE_EXIT(x) ModuleExit g_pModuleExit __attribute__((section(".modinfo"))) = x
#define MODULE_DEPENDS(...) const char *g_pDepends[] __attribute__((section(".modinfo"))) = {__VA_ARGS__, 0}

#endif
