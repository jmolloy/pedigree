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

#include <Log.h>
#include <vfs/VFS.h>
#include <vfs/Directory.h>
#include "../../subsys/posix/PosixSubsystem.h"
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
#include <utilities/assert.h>

#include <kernel/core/BootIO.h>

#include <network/NetworkStack.h>
#include <network/RoutingTable.h>

#include <users/UserManager.h>

#include <utilities/TimeoutGuard.h>

#include <config/ConfigurationManager.h>
#include <config/MemoryBackend.h>

#include <machine/DeviceHashTree.h>
#include <lodisk/LoDisk.h>

extern void pedigree_init_sigret();
extern void pedigree_init_pthreads();

extern BootIO bootIO;

void init_stage2();

static bool bRootMounted = false;
static bool probeDisk(Disk *pDisk)
{
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

void hashesoneoneone(Device *pDev)
{
}

void init()
{
    static HugeStaticString str;

    // Mount all available filesystems.
    if (!findDisks(&Device::root()))
    {
//     FATAL("No disks found!");
    }

//    if (VFS::instance().find(String("raw»/")) == 0)
//    {
//        FATAL("No raw partition!");
//    }

    // Are we running a live CD?
    /// \todo Use the configuration manager to determine if we're running a live CD or
    ///       not, to avoid the potential for conflicts here.
    if(VFS::instance().find(String("root»/livedisk.img")))
    {
        FileDisk *pRamDisk = new FileDisk(String("root»/livedisk.img"), FileDisk::RamOnly);
        if(pRamDisk && pRamDisk->initialise())
        {
            pRamDisk->setParent(&Device::root());
            Device::root().addChild(pRamDisk);

            // Mount it in the VFS
            VFS::instance().removeAlias(String("root"));
            bRootMounted = false;
            findDisks(pRamDisk);
        }
        else
            delete pRamDisk;
    }

    // Is there a root disk mounted?
    if(VFS::instance().find(String("root»/.pedigree-root")) == 0)
    {
        FATAL("No root disk (missing .pedigree-root?)");
    }

    // Fill out the device hash table
    DeviceHashTree::instance().fill(&Device::root());

#if 0
    // Testing froggey's Bochs patch for magic watchpoints... -Matt
    volatile uint32_t abc = 0;
    NOTICE("Address of abc = " << reinterpret_cast<uintptr_t>(&abc) << "...");
    asm volatile("xchg %%cx,%%cx" :: "a" (&abc));
    abc = 0xdeadbeef;
    abc = 0xdeadbe;
    abc = 0xdead;
    abc = 0xde;
    abc = 0xd;
    abc = 0;
    FATAL("Test complete: " << abc << ".");
#endif

    // Initialise user/group configuration.
    UserManager::instance().initialise();

    // Build routing tables - try to find a default configuration that can
    // connect to the outside world
    IpAddress empty;
    bool bRouteFound = false;
    for (size_t i = 0; i < NetworkStack::instance().getNumDevices(); i++)
    {
        /// \todo Perhaps try and ping a remote host?
        Network* card = NetworkStack::instance().getDevice(i);
        StationInfo info = card->getStationInfo();

        // If the device has a gateway, set it as the default and continue
        if (info.gateway != empty)
        {
            if(!bRouteFound)
            {
                RoutingTable::instance().Add(RoutingTable::Named, empty, empty, String("default"), card);
                bRouteFound = true;
            }

            // Additionally route the complement of its subnet to the gateway
            RoutingTable::instance().Add(RoutingTable::DestSubnetComplement,
                                         info.ipv4,
                                         info.subnetMask,
                                         info.gateway,
                                         String(""),
                                         card);

            // And the actual subnet that the card is on needs to route to... the card.
            RoutingTable::instance().Add(RoutingTable::DestSubnet,
                                         info.ipv4,
                                         info.subnetMask,
                                         empty,
                                         String(""),
                                         card);
        }

        // If this isn't already the loopback device, redirect our own IP to 127.0.0.1
        if(info.ipv4.getIp() != Network::convertToIpv4(127, 0, 0, 1))
            RoutingTable::instance().Add(RoutingTable::DestIpSub, info.ipv4, Network::convertToIpv4(127, 0, 0, 1), String(""), NetworkStack::instance().getLoopback());
        else
            RoutingTable::instance().Add(RoutingTable::DestIp, info.ipv4, empty, String(""), card);
    }

    // Otherwise, just assume the default is interface zero
    if (!bRouteFound)
        RoutingTable::instance().Add(RoutingTable::Named, empty, empty, String("default"), NetworkStack::instance().getDevice(0));

    str += "Loading init program (root»/applications/login)\n";
    bootIO.write(str, BootIO::White, BootIO::Black);
    str.clear();

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

    PosixSubsystem *pSubsystem = new PosixSubsystem;
    pProcess->setSubsystem(pSubsystem);

    new Thread(pProcess, reinterpret_cast<Thread::ThreadStartFunc>(&init_stage2), 0x0 /* parameter */);

    lock.release();
}
void destroy()
{
}

extern void system_reset();

void init_stage2()
{
    // Load initial program.
    File* initProg = VFS::instance().find(String("root»/applications/TUI"));
    if (!initProg)
    {
        NOTICE("INIT: FileNotFound!!");
        initProg = VFS::instance().find(String("root»/applications/tui"));
        if (!initProg)
        {
            FATAL("Unable to load init program!");
            return;
        }
    }
    NOTICE("INIT: File found: " << reinterpret_cast<uintptr_t>(initProg));
    String fname = initProg->getName();
    NOTICE("INIT: name: " << fname);
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

    // Initialise the sigret and pthreads shizzle.
    pedigree_init_sigret();
    pedigree_init_pthreads();

#if 0
    system_reset();
#else
    // Alrighty - lets create a new thread for this program - -8 as PPC assumes the previous stack frame is available...
    new Thread(pProcess, reinterpret_cast<Thread::ThreadStartFunc>(pLinker->getProgramElf()->getEntryPoint()), 0x0 /* parameter */,  reinterpret_cast<void*>(0x20020000-8) /* Stack */);
#endif
}

MODULE_NAME("init");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
#ifdef X86_COMMON
//MODULE_DEPENDS("VFS", "ext2", "posix", "partition", "TUI", "linker", "vbe", "NetworkStack", "users", "rawfs");
MODULE_DEPENDS("VFS", "ext2", "posix", "partition", "TUI", "linker", "vbe", "users");
#elif PPC_COMMON
MODULE_DEPENDS("VFS", "ext2", "posix", "partition", "TUI", "linker", "NetworkStack", "users");
#endif

