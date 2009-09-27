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
#include <FbdevFs.h>
#include <Log.h>
#include <utilities/assert.h>
#include <Module.h>
#include <vfs/Directory.h>
#include <vfs/VFS.h>
#include "../vbe/VbeDisplay.h"

extern VbeDisplay *g_pDisplays[];
extern size_t g_nDisplays;

FbdevFs::FbdevFs() :
    m_pRoot(0)
{
    m_pRoot = new Directory(String("root"), 0, 0, 0, FBDEV_INODE_ROOT, this, 0, 0);
}

FbdevFs::~FbdevFs()
{
    delete m_pRoot;
}

uint64_t FbdevFs::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    FATAL("READ, modda fokka!");
    return 0;
}

uint64_t FbdevFs::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    VbeDisplay *pDisplay = g_pDisplays[pFile->getInode()];

    uint8_t *pFb = reinterpret_cast<uint8_t*>(pDisplay->getFramebuffer());
    memcpy(pFb+location, reinterpret_cast<uint8_t*>(buffer), size);
    return size;
}

void FbdevFs::cacheDirectoryContents(File *pFile)
{
    assert(pFile->getInode() == FBDEV_INODE_ROOT);
    Directory *pDir = Directory::fromFile(pFile);

    for (size_t i = 0; i < g_nDisplays; i++)
    {
        /// \bug m_Cache.insert does NOT like strings made with sprintf...
        String str("display0");
        //str.sprintf("display%d", i);
        pDir->m_Cache.insert(str, new File(str, 0, 0, 0, i, this, 0, pFile));
    }
    pDir->m_bCachePopulated = true;
}

FbdevFs g_FbdevFs;

void init()
{
    VFS::instance().addAlias(&g_FbdevFs, String("fbdev"));
}

void destroy()
{
}

MODULE_NAME("fbdev");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
MODULE_DEPENDS("VFS","vbe");
