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

#include <utilities/string.h>

typedef void (*ModuleEntry)();
typedef void (*ModuleExit)();

#ifdef STATIC_DRIVERS

// Arbitary limits, so sue me...
#define MAX_DEPENDENCIES    32
#define MAX_NAME_LENGTH     256

struct ModuleInfo
{
    ModuleInfo(const char *newName, ModuleEntry newEntry, ModuleExit newExit, const char **newDeps)
    {
        strcpy(name, newName);
        entry = newEntry;
        exit = newExit;
        dependencies = newDeps;
    }

    char name[MAX_NAME_LENGTH];
    ModuleEntry entry;
    ModuleExit exit;
    const char **dependencies; // [MAX_DEPENDENCIES][MAX_NAME_LENGTH];
};

#define MODULE_INFO(name, entry, exit, deps) static ModuleInfo __module __attribute__((section(".modinfo"))) (name, entry, exit, deps);

#else

#define MODULE_NAME(x) const char *g_pModuleName __attribute__((section(".modinfo"))) = x
#define MODULE_ENTRY(x) ModuleEntry g_pModuleEntry __attribute__((section(".modinfo"))) = x
#define MODULE_EXIT(x) ModuleExit g_pModuleExit __attribute__((section(".modinfo"))) = x
#define MODULE_DEPENDS(...) const char *g_pDepends[] __attribute__((section(".modinfo"))) = {__VA_ARGS__, 0}

#define MODULE_INFO(name, entry, exit, ...) do { \
                                                MODULE_NAME(name); \
                                                MODULE_ENTRY(entry); \
                                                MODULE_EXIT(exit); \
                                                MODULE_DEPENDS(__VA_ARGS__); \
                                            } while(0);

#endif

#endif
