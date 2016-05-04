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

#include <utilities/List.h>

#include "Module.h"

#ifndef STATIC_DRIVERS

extern "C"
{

typedef void (*__cxa_atexit_func_type)(void *);

struct __cxa_atexit_record
{
    __cxa_atexit_func_type func;
    void *arg;
};

static List<__cxa_atexit_record> *g_AtExitEntries = 0;

// We provide an implementation of __cxa_atexit for each module, so that we can
// run the finalizers correctly.
void __cxa_atexit(void (*f)(void *), void *arg, void *dso_handle)
{
    if (!g_AtExitEntries)
    {
        g_AtExitEntries = new List<__cxa_atexit_record>;
    }

    __cxa_atexit_record rec;
    rec.func = f;
    rec.arg = arg;

    g_AtExitEntries->pushBack(rec);
}

// This is the lowest priority we can assign a destructor. This runs on
// termination to call all the atexit()s set up by __cxa_atexit above.
void __perform_module_exit() __attribute__((destructor(101)));
void __perform_module_exit()
{
    if (!g_AtExitEntries)
        return;

    for (auto it = g_AtExitEntries->begin(); it != g_AtExitEntries->end(); ++it)
    {
        (*it).func((*it).arg);
    }

    delete g_AtExitEntries;
}

}  // extern "C"

#endif
