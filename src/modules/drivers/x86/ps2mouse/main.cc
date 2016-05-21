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

#include <Module.h>
#include "Ps2Mouse.h"

// Global static object for the PS/2 mouse we'll be working with
static Ps2Mouse *g_Ps2Mouse = 0;

static bool entry()
{
    auto f = [] (Device *p) {
        if (g_Ps2Mouse)
        {
            return p;
        }

        if(p->addresses().count() > 0)
        {
            if(p->addresses()[0]->m_Name == "ps2-base")
            {
                Ps2Mouse *pNewChild = new Ps2Mouse(p);
                if (pNewChild->initialise(p->addresses()[0]->m_Io))
                {
                    g_Ps2Mouse = pNewChild;
                }
                else
                {
                    ERROR("PS/2 Mouse initialisation failed!");
                    delete pNewChild;
                }
            }
        }

        return p;
    };

    auto c = pedigree_std::make_callable(f);
    Device::foreach(c, 0);

    // Cannot replace the child, as we need to have it present for keyboards.
    if (g_Ps2Mouse)
    {
        Device::addToRoot(g_Ps2Mouse);
        return true;
    }

    return false;
}

static void unload()
{
    if(g_Ps2Mouse)
        delete g_Ps2Mouse;
}

MODULE_NAME("ps2mouse");
MODULE_ENTRY(&entry);
MODULE_EXIT(&unload);
MODULE_DEPENDS(0);
