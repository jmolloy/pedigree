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

#include <Log.h>
#include <Module.h>
#include <machine/Disk.h>
#include <vfs/VFS.h>
#include <core/BootIO.h>
#include <lodisk/LoDisk.h>

static bool bRootMounted = false;

static void error(const char *s)
{
    extern BootIO bootIO;
    static HugeStaticString str;
    str += s;
    str += "\n";
    bootIO.write(str, BootIO::Red, BootIO::Black);
    str.clear();
}

static Device *probeDisk(Device *diskDevice)
{
    if (diskDevice->getType() != Device::Disk)
    {
        return diskDevice;
    }

    Disk *pDisk = static_cast<Disk *>(diskDevice);
    String alias; // Null - gets assigned by the filesystem.
    if (VFS::instance().mount(pDisk, alias))
    {
        // For mount message
        bool didMountAsRoot = false;

        // Search for the root specifier, if we haven't already mounted root
        if (!bRootMounted)
        {
            NormalStaticString s;
            s += alias;
            s += "»/.pedigree-root";

            File* f = VFS::instance().find(String(static_cast<const char*>(s)));
            if (f && !bRootMounted)
            {
                NOTICE("Mounted " << alias << " successfully as root.");
                VFS::instance().addAlias(alias, String("root"));
                bRootMounted = didMountAsRoot = true;
            }
        }

        if(!didMountAsRoot)
        {
            NOTICE("Mounted " << alias << ".");
        }
    }

    return diskDevice;
}

static bool init()
{
    // Mount all available filesystems.
    Device::foreach(probeDisk);

    if (VFS::instance().find(String("raw»/")) == 0)
    {
        error("raw» does not exist - cannot continue startup.");
        return false;
    }

    // Are we running a live CD?
    /// \todo Use the configuration manager to determine if we're running a live CD or
    ///       not, to avoid the potential for conflicts here.
    if(VFS::instance().find(String("root»/livedisk.img")))
    {
        FileDisk *pRamDisk = new FileDisk(String("root»/livedisk.img"), FileDisk::RamOnly);
        if(pRamDisk && pRamDisk->initialise())
        {
            Device::addToRoot(pRamDisk);

            // Mount it in the VFS
            VFS::instance().removeAlias(String("root"));
            bRootMounted = false;
            Device::foreach(probeDisk, pRamDisk);
        }
        else
            delete pRamDisk;
    }

    // Is there a root disk mounted?
    if(VFS::instance().find(String("root»/.pedigree-root")) == 0)
    {
        error("No root disk on this system (no root»/.pedigree-root found).");
        return false;
    }

    // All done, nothing more to do here.
    return true;
}

static void destroy()
{
    NOTICE("Unmounting all filesystems...");

    Tree<Filesystem *, List<String *> *> &mounts = VFS::instance().getMounts();
    List<Filesystem *> deletionQueue;

    for (auto it = mounts.begin(); it != mounts.end(); ++it)
    {
        deletionQueue.pushBack(it.key());
    }

    while (deletionQueue.count())
    {
        Filesystem *pFs = deletionQueue.popFront();
        NOTICE("Unmounting " << pFs->getVolumeLabel() << " [" << pFs << "]...");
        VFS::instance().removeAllAliases(pFs);
        NOTICE("unmount done");
    }

    NOTICE("Unmounting all filesystems has completed.");
}

MODULE_INFO("mountroot", &init, &destroy, "vfs", "partition");

// We expect the filesystems metamodule to fail, but by the time it does and
// we are allowed to continue, all the filesystems are loaded.
MODULE_OPTIONAL_DEPENDS("filesystems");
