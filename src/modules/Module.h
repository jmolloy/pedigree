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

/**
 * \page module_main Module System
 * Pedigree's module system provides the foundation upon which all drivers and
 * loadable system functionality are built. Modules can be loaded automatically
 * at startup by inserting them into the system initrd, or they can be loaded
 * after system startup via system calls. Modules can also be unloaded after
 * system startup completes.
 *
 * \section module_overview Overview
 * Pedigree modules are simply ELF object files that have not yet completed a
 * final link stage. \ref kernellinker "Pedigree's module loader" handles the
 * final link when loading, fixing up symbol references against the kernel
 * symbol table. This allows modules to use all symbols already available in the
 * kernel. This also allows modules to use symbols defined by previously-loaded
 * modules.
 *
 * Each module provides functions to perform initialisation and termination,
 * which are called by the kernel when the module is loaded and unloaded,
 * respectively. Modules may also define dependencies, which must be loaded
 * before attempting to load the module.
 *
 * Modules are identified by a unique name, which is an ASCII string.
 *
 * \section module_howto Creating a Module
 * The process for creating a module is fairly simple.
 *
 * Firstly, determine whether or not the module is a driver or a system module.
 * The distinction is not technically important, but modules in the repository
 * are split between drivers and system modules. If the module is a driver,
 * it belongs either in the 'common' directory, if architecture-independent, or
 * in the relevant directory for the target architecture.
 *
 * Create a module directory in the relevant place (as decided above), and
 * create at a minimum a single C++ file that will define the basic requirements
 * for the module.
 *
 * In this C++ file, include \ref Module.h and invoke the `MODULE_INFO` macro.
 * This macro defines the necessary metadata to allow the module to be loaded.
 *
 *     static bool example_entry() { return true; }
 *     static void example_exit() {}
 *     MODULE_INFO("example", example_entry, example_exit, "vfs", "init");
 *
 * This will define a module "example", and call the `example_entry` function
 * upon loading the module. The module will only be loaded once the "vfs" and
 * "init" modules have been loaded. When the module is unloaded, the
 * `example_exit` function will be called.
 *
 * Note the fact that `example_entry` returns `bool`. Should the module entry
 * function return `false`, the module is automatically unloaded. This is useful
 * for drivers which do hardware detection as they can be removed from memory
 * if the system does not have a particular piece of hardware.
 *
 * \section module_static Static Modules
 * For some architectures, it makes more sense to build modules into the kernel
 * rather than provide them in an initrd. For other architectures, the necessary
 * support to load modules from an initrd has not yet been written. In these
 * cases, `STATIC_DRIVERS` will be defined in the preprocessor. \ref Module.h
 * already handles this case and `MODULE_INFO` will do the correct thing.
 *
 * There should be essentially no difference to module code; it is not expected
 * that `STATIC_DRIVERS` is a preprocessor definition that needs to be checked
 * by module code.
 */

#ifndef MODULE_H
#define MODULE_H

#include <compiler.h>
#include <utilities/utility.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef bool (*ModuleEntry)();
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
} PACKED;

#define MODULE_INFO2(name, entry, exit, ...) \
    static const char *__mod_deps[] = {__VA_ARGS__, 0}; \
    static ModuleInfo __module SECTION(".modinfo") (name, entry, exit, __mod_deps);

#else

#define MODULE_NAME(x) const char *g_pModuleName SECTION(".modinfo") = x
#define MODULE_ENTRY(x) ModuleEntry g_pModuleEntry SECTION(".modinfo") = x
#define MODULE_EXIT(x) ModuleExit g_pModuleExit SECTION(".modinfo") = x
#define MODULE_DEPENDS(...) const char *g_pDepends[] SECTION(".modinfo") = {__VA_ARGS__, 0}
#define MODULE_DEPENDS2(...) const char *g_pDepends[] SECTION(".modinfo") = {__VA_ARGS__}

#define MODULE_OPTIONAL_DEPENDS(...) const char *g_pOptionalDepends[] SECTION(".modinfo") = {__VA_ARGS__, 0}

#define MODULE_INFO2(name, entry, exit, ...) MODULE_NAME(name); \
                                            MODULE_ENTRY(entry); \
                                            MODULE_EXIT(exit); \
                                            MODULE_DEPENDS2(__VA_ARGS__);

#endif

#define MODULE_INFO(...) MODULE_INFO2(__VA_ARGS__, 0)

#endif
