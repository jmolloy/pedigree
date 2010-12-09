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

#include "RawFs.h"
#include "RawFsFile.h"
#include "RawFsDir.h"

#include <Module.h>
#include <processor/Processor.h>

RawFs *g_pRawFs;

RawFs::RawFs() :
    m_pRoot(0)
{
    m_pRoot = new RawFsDir(String(""),
                           this,
                           0);
}

RawFs::~RawFs()
{
    if(m_pRoot)
        delete m_pRoot;
}

File *RawFs::getRoot()
{
    return m_pRoot;
}

bool hasDiskChildren(Device *pDev)
{
    for (size_t i = 0; i < pDev->getNumChildren(); i++)
    {
        Device *pChild = pDev->getChild(i);
        if (pChild->getType() == Device::Disk)
            return true;
        if (hasDiskChildren(pChild))
            return true;
    }
    return false;
}

void searchNode(Device *pDev, RawFsDir *pDir)
{
    for (size_t i = 0; i < pDev->getNumChildren(); i++)
    {
        Device *pChild = pDev->getChild(i);
        if (pChild->getType() == Device::Disk)
        {
            String str;
            pChild->getName(str);
            if (hasDiskChildren(pChild))
            {
                NOTICE("rawfs: adding dir '" << str << "'");
                RawFsDir *pDir2 = new RawFsDir(str,
                                               g_pRawFs,
                                               static_cast<File*>(pDir));
                pDir->addEntry(pDir2);
                pDir->addEntry(new RawFsFile(String("entire-disk"),
                                             g_pRawFs,
                                            static_cast<File*>(pDir),
                                             reinterpret_cast<Disk*>(pChild)));
                searchNode(pChild, pDir2);
            }
            else
            {
                NOTICE("rawfs: adding file '" << str << "'");
                pDir->addEntry(new RawFsFile(str,
                                             g_pRawFs,
                                             static_cast<File*>(pDir),
                                             reinterpret_cast<Disk*>(pChild)));
            }
        }
        else
            searchNode(pChild, pDir);
    }
}

void rescanTree()
{
    RawFsDir *pD = static_cast<RawFsDir*>(g_pRawFs->getRoot());
    pD->removeRecursive();

    // Find the disk topology in an intermediate form.
    searchNode(&Device::root(), pD);
}

void init()
{
    g_pRawFs = new RawFs();
    VFS::instance().addMountCallback(&rescanTree);
    rescanTree();
    String alias("raw");
    VFS::instance().addAlias(g_pRawFs, alias);
}

void destroy()
{
}

MODULE_INFO("rawfs", &init, &destroy, "vfs");
