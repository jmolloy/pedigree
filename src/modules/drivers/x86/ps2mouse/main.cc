/*
 * Copyright (c) 2010 Matthew Iselin
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

Device *findPs2Mouse(Device *base)
{
    for (unsigned int i = 0; i < base->getNumChildren(); i++)
    {
        Device *pChild = base->getChild(i);

        // Check that this device actually has IO regions
        if(pChild->addresses().count() > 0)
            if(pChild->addresses()[0]->m_Name == "ps2-base")
                return pChild;
        
        // If the device is a PS/2 base, we won't get here. So recurse.
        if(pChild->getNumChildren())
        {
            Device *p = findPs2Mouse(pChild);
            if(p)
                return p;
        }
    }
    return 0;
}

void entry()
{
    Device *root = &Device::root();
    Device *pDevice = findPs2Mouse(root);
    if(pDevice)
    {
        g_Ps2Mouse = new Ps2Mouse(pDevice);

        if(g_Ps2Mouse->initialise(pDevice->addresses()[0]->m_Io))
        {
            g_Ps2Mouse->setParent(root);
            root->addChild(g_Ps2Mouse);
        }
        else
        {
            delete g_Ps2Mouse;
            g_Ps2Mouse = 0;
        }
    }
}

void unload()
{
    if(g_Ps2Mouse)
        delete g_Ps2Mouse;
}

MODULE_NAME("ps2mouse");
MODULE_ENTRY(&entry);
MODULE_EXIT(&unload);
MODULE_DEPENDS(0);
