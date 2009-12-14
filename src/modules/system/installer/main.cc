/*
 * Copyright (c) 2008 James Molloy, JÃ¶rg PfÃ¤hler, Matthew Iselin
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
#include <vfs/VFS.h>
#include <vfs/Directory.h>
#include <machine/Device.h>
#include <machine/Disk.h>
#include <Module.h>
#include <processor/Processor.h>
#include <linker/Elf.h>
#include <process/Thread.h>
#include <process/Process.h>
#include <process/Scheduler.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <machine/Machine.h>
#include <linker/DynamicLinker.h>
#include <panic.h>

#include <kernel/core/BootIO.h>

#include <users/UserManager.h>

#include <utilities/TimeoutGuard.h>

#include <ramfs/RamFs.h>
#include <lodisk/LoDisk.h>

extern BootIO bootIO;

void init_stage2();

static bool probeDisk(Disk *pDisk)
{
    String alias; // Null - gets assigned by the filesystem.
    static bool bRootMounted = false;
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
        return false;
    }
    return false;
}

static bool findDisks(Device *pDev)
{
    for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
    {
        Device *pChild = pDev->getChild(i);
        if (pChild->getNumChildren() == 0 && /* Only check leaf nodes. */
                pChild->getType() == Device::Disk)
        {
            if ( probeDisk(static_cast<Disk*> (pChild)) ) return true;
        }
        else
        {
            // Recurse.
            if (findDisks(pChild)) return true;
        }
    }
    return false;
}

void init_install()
{
    static HugeStaticString str;

    // Mount all available filesystems.
    if (!findDisks(&Device::root()))
    {
//     FATAL("No disks found!");
    }

    if (VFS::instance().find(String("raw»/")) == 0)
    {
        FATAL("No raw partition!");
    }

    // Setup the RAMFS for the mount table before the real HDDs. Doing it early
    // means if someone happens to have a disk with a volume label that matches
    // "ramfs", we don't end up failing later.
    FileDisk *pRamDisk = new FileDisk(String("PEDIGREEINSTALL»/disk.img"), FileDisk::RamOnly);
    if(pRamDisk && pRamDisk->initialise())
        NOTICE("The installer module got a ramdisk from disk.img.");
    else
        FATAL("Couldn't initialise ramdisk!");

    // Shove the ramdisk into the device tree somewhere
    pRamDisk->setParent(&Device::root());
    Device::root().addChild(pRamDisk);

    // Mount it in the VFS
    findDisks(pRamDisk);

    // Build the mount table
    if (!VFS::instance().createFile(String("root»/mount.tab"), 0777, 0))
        FATAL("Couldn't create mount table in the RAMFS");
    File* mountTab = VFS::instance().find(String("root»/mount.tab"));

    List<VFS::Alias*> myAliases = VFS::instance().getAliases();
    NormalStaticString myMounts;
    for (List<VFS::Alias*>::Iterator i = myAliases.begin(); i != myAliases.end(); i++)
    {
        // Check for the mounts upon which we *don't* want Pedigree to be installed on
        /// \todo Find a better way of doing this...
        String alias = (*i)->alias;
        if (!strncmp(static_cast<const char*>(alias), "PEDIGREEINSTALL", strlen("PEDIGREEINSTALL")))
            continue;
        if (!strncmp(static_cast<const char*>(alias), "root", strlen("root")))
            continue;
        if (!strncmp(static_cast<const char*>(alias), "insramfs", strlen("insramfs")))
            continue;
        if (!strncmp(static_cast<const char*>(alias), "raw", strlen("raw")))
            continue;

        myMounts += alias;
        myMounts += '\n';
    }

    // Write to the file
    mountTab->write(0, myMounts.length(), reinterpret_cast<uintptr_t>(static_cast<const char*>(myMounts)));

    mountTab->decreaseRefCount(true);

    NOTICE("Available mounts for install:\n" << String(myMounts));

    // Initialise user/group configuration.
    UserManager::instance().initialise();

    str += "Loading init program (root»/applications/TUI)\n";
    bootIO.write(str, BootIO::White, BootIO::Black);
    str.clear();

    Machine::instance().getKeyboard()->setDebugState(false);

    // At this point we're uninterruptible, as we're forking.
    Spinlock lock;
    lock.acquire();

    // Create a new process for the init process.
    Process *pProcess = new Process(Processor::information().getCurrentThread()->getParent());
    pProcess->setUser(UserManager::instance().getUser(0));
    pProcess->setGroup(UserManager::instance().getUser(0)->getDefaultGroup());

    pProcess->description().clear();
    pProcess->description().append("init");

    pProcess->setCwd(VFS::instance().find(String("root»/")));
    pProcess->setCtty(0);

    new Thread(pProcess, reinterpret_cast<Thread::ThreadStartFunc>(&init_stage2), 0x0 /* parameter */);

    lock.release();
}

void destroy_install()
{
}

void init_stage2()
{
    // Load initial program.
    File* initProg = VFS::instance().find(String("root»/applications/TUI"));
    if (!initProg)
    {
        initProg = VFS::instance().find(String("root»/applications/tui"));
        if (!initProg)
        {
            FATAL("Unable to load init program!");
            return;
        }
    }

    // That will have forked - we don't want to fork, so clear out all the chaff in the new address space that's not
    // in the kernel address space so we have a clean slate.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    pProcess->getAddressSpace()->revertToKernelAddressSpace();

    DynamicLinker *pLinker = new DynamicLinker();
    pProcess->setLinker(pLinker);

    if (!pLinker->loadProgram(initProg))
    {
        FATAL("Init program failed to load!");
    }

    for (int j = 0; j < 0x20000; j += 0x1000)
    {
        physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
        bool b = Processor::information().getVirtualAddressSpace().map(phys, reinterpret_cast<void*> (j+0x20000000), VirtualAddressSpace::Write);
        if (!b)
            WARNING("map() failed in init");
    }


    ////////////////////////////////////////////////////////////////////
    // Temp!
#if 0
    Semaphore sem(0);

    sem.acquire(1, 1);
    NOTICE("Acquired sem: " << Processor::information().getCurrentThread()->wasInterrupted());

    NOTICE("Ended!");
    for (;;);
#endif
    ////////////////////////////////////////////////////////////////////
    // End Temp!

    // Alrighty - lets create a new thread for this program - -8 as PPC assumes the previous stack frame is available...
    new Thread(pProcess, reinterpret_cast<Thread::ThreadStartFunc>(pLinker->getProgramElf()->getEntryPoint()), 0x0 /* parameter */,  reinterpret_cast<void*>(0x20020000-8) /* Stack */);
}

MODULE_NAME("installer");
MODULE_ENTRY(&init_install);
MODULE_EXIT(&destroy_install);
#ifdef X86_COMMON
MODULE_DEPENDS("vfs", "ext2", "posix", "partition", "TUI", "linker", "vbe", "network-stack", "users", "lodisk");
#elif PPC_COMMON
MODULE_DEPENDS("vfs", "ext2", "posix", "partition", "TUI", "linker", "network-stack", "users", "lodisk");
#endif

