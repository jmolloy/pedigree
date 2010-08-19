/*
 * Copyright (c) 2010 James Molloy, Jörg Pfähler, Matthew Iselin
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

#include <utilities/utility.h>

typedef void (*ModuleEntry)();
typedef void (*ModuleExit)();

#ifdef STATIC_DRIVERS

#define MODULE_TAG          0xdeadbaba

struct ModuleInfo
{
    ModuleInfo(const char *newName, ModuleEntry newEntry, ModuleExit newExit, const char **newDeps)
    {
        tag = MODULE_TAG;
        name = newName;
        entry = newEntry;
        exit = newExit;
        dependencies = newDeps;
    }

    uint32_t tag;
    const char *name;
    ModuleEntry entry;
    ModuleExit exit;
    const char **dependencies;
};

#define MODULE_INFO2(name, entry, exit, ...) \
    static const char *__mod_deps[] = {__VA_ARGS__}; \
    static ModuleInfo __module __attribute__((section(".modinfo"))) (name, entry, exit, __mod_deps);

#else

#define MODULE_NAME(x) const char *g_pModuleName __attribute__((section(".modinfo"))) = x
#define MODULE_ENTRY(x) ModuleEntry g_pModuleEntry __attribute__((section(".modinfo"))) = x
#define MODULE_EXIT(x) ModuleExit g_pModuleExit __attribute__((section(".modinfo"))) = x
#define MODULE_DEPENDS(...) const char *g_pDepends[] __attribute__((section(".modinfo"))) = {__VA_ARGS__, 0}
#define MODULE_DEPENDS2(...) const char *g_pDepends[] __attribute__((section(".modinfo"))) = {__VA_ARGS__}

#define MODULE_INFO2(name, entry, exit, ...) MODULE_NAME(name); \
                                            MODULE_ENTRY(entry); \
                                            MODULE_EXIT(exit); \
                                            MODULE_DEPENDS2(__VA_ARGS__);

#endif

#define MODULE_INFO(...) MODULE_INFO2(__VA_ARGS__, 0)

#endif
