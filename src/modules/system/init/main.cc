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
#include <vfs/VFS.h>
#include <vfs/Directory.h>
#include <subsys/posix/PosixSubsystem.h> // In src
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

#include <core/BootIO.h> // In src/system/kernel

#include <network-stack/NetworkStack.h>
#include <network-stack/RoutingTable.h>
#include <network-stack/UdpLogger.h>

#include <ramfs/RamFs.h>

#include <users/UserManager.h>

#include <utilities/TimeoutGuard.h>

#include <config/ConfigurationManager.h>
#include <config/MemoryBackend.h>

#include <machine/DeviceHashTree.h>
#include <lodisk/LoDisk.h>

#include <machine/InputManager.h>

#include <ServiceManager.h>

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

/// This ensures that the init module entry function will not exit until the init
/// program entry point thread is running.
Mutex g_InitProgramLoaded(true);

static bool init()
{
    static HugeStaticString str;

    // Mount all available filesystems.
    findDisks(&Device::root());

    // Mount scratch filesystem (ie, pure ram filesystem, for POSIX /tmp etc)
    RamFs *pRamFs = new RamFs;
    pRamFs->initialise(0);
    VFS::instance().addAlias(pRamFs, String("scratch"));

    // Mount runtime filesystem.
    // The runtime filesystem assigns a Process ownership to each file, only
    // that process can modify/remove it. If the Process terminates without
    // removing the file, the file is not removed.
    RamFs *pRuntimeFs = new RamFs;
    pRuntimeFs->initialise(0);
    pRuntimeFs->setProcessOwnership(true);
    VFS::instance().addAlias(pRuntimeFs, String("runtime"));

    if (VFS::instance().find(String("raw»/")) == 0)
    {
        FATAL("No raw partition!");
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
        return false;
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
    Network *pDefaultCard = 0;
    for (size_t i = 0; i < NetworkStack::instance().getNumDevices(); i++)
    {
        /// \todo Perhaps try and ping a remote host?
        Network* card = NetworkStack::instance().getDevice(i);

        StationInfo info = card->getStationInfo();

        // IPv6 stateless autoconfiguration and DHCP/DHCPv6 must not happen on
        // the loopback device, which has a fixed address.
        if(info.ipv4.getIp() != Network::convertToIpv4(127, 0, 0, 1))
        {
            // Auto-configure IPv6 on this card.
            ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("ipv6"));
            Service         *pService  = ServiceManager::instance().getService(String("ipv6"));
            if(pFeatures->provides(ServiceFeatures::touch))
                if(pService)
                    pService->serve(ServiceFeatures::touch, reinterpret_cast<void*>(card), sizeof(*card));

            // Ask for a DHCP lease on this card
            /// \todo Static configuration
            pFeatures = ServiceManager::instance().enumerateOperations(String("dhcp"));
            pService  = ServiceManager::instance().getService(String("dhcp"));
            if(pFeatures->provides(ServiceFeatures::touch))
                if(pService)
                    pService->serve(ServiceFeatures::touch, reinterpret_cast<void*>(card), sizeof(*card));
        }

        StationInfo newInfo = card->getStationInfo();

        // List IPv6 addresses
        for(size_t i = 0; i < info.nIpv6Addresses; i++)
            NOTICE("Interface " << i << " has IPv6 address " << info.ipv6[i].toString() << " (" << Dec << i << Hex << " out of " << info.nIpv6Addresses << ")");

        // If the device has a gateway, set it as the default and continue
        if (newInfo.gateway != empty)
        {
            if(!pDefaultCard)
                pDefaultCard = card;

            // Additionally route the complement of its subnet to the gateway
            RoutingTable::instance().Add(RoutingTable::DestSubnetComplement,
                                         newInfo.ipv4,
                                         newInfo.subnetMask,
                                         newInfo.gateway,
                                         String(""),
                                         card);
        }

        // And the actual subnet that the card is on needs to route to... the card.
        RoutingTable::instance().Add(RoutingTable::DestSubnet,
                newInfo.ipv4,
                newInfo.subnetMask,
                empty,
                String(""),
                card);

        // If this isn't already the loopback device, redirect our own IP to 127.0.0.1
        if(newInfo.ipv4.getIp() != Network::convertToIpv4(127, 0, 0, 1))
            RoutingTable::instance().Add(RoutingTable::DestIpSub, newInfo.ipv4, Network::convertToIpv4(127, 0, 0, 1), String(""), NetworkStack::instance().getLoopback());
        else
            RoutingTable::instance().Add(RoutingTable::DestIp, newInfo.ipv4, empty, String(""), card);
    }

    // Otherwise, just assume the default is interface zero
    if (!pDefaultCard)
        RoutingTable::instance().Add(RoutingTable::Named, empty, empty, String("default"), NetworkStack::instance().getDevice(0));
    else
        RoutingTable::instance().Add(RoutingTable::Named, empty, empty, String("default"), pDefaultCard);

#if 0
    // Routes installed, start the UDP logger
    UdpLogger *logger = new UdpLogger();
    logger->initialise(IpAddress(Network::convertToIpv4(192, 168, 0, 1)));

    Log::instance().installCallback(logger);
#endif

    str += "Loading init program (root»/applications/init)\n";
    bootIO.write(str, BootIO::White, BootIO::Black);
    str.clear();

#ifdef HOSTED
    return true;
#endif

#ifdef THREADS
    // At this point we're uninterruptible, as we're forking.
    Spinlock lock;
    lock.acquire();

    // Create a new process for the init process.
    Process *pProcess = new Process(Processor::information().getCurrentThread()->getParent());
    pProcess->setUser(UserManager::instance().getUser(0));
    pProcess->setGroup(UserManager::instance().getUser(0)->getDefaultGroup());
    pProcess->setEffectiveUser(pProcess->getUser());
    pProcess->setEffectiveGroup(pProcess->getGroup());

    pProcess->description().clear();
    pProcess->description().append("init");

    pProcess->setCwd(VFS::instance().find(String("root»/")));
    pProcess->setCtty(0);

    PosixSubsystem *pSubsystem = new PosixSubsystem;
    pProcess->setSubsystem(pSubsystem);

    Thread *pThread = new Thread(pProcess, reinterpret_cast<Thread::ThreadStartFunc>(&init_stage2), 0x0 /* parameter */);
    pThread->detach();

    lock.release();

    // Wait for the program to load
    g_InitProgramLoaded.acquire();
#else
    #warning the init module is almost useless without threads.
#endif

    return true;
}
static void destroy()
{
}

extern void system_reset();

void init_stage2()
{
    // Load initial program.
    String fname = String("root»/applications/init");
    File* initProg = VFS::instance().find(fname);
    if (!initProg)
    {
        FATAL("INIT: FileNotFound!!");
        return;
    }
    NOTICE("INIT: File found");
    NOTICE("INIT: name: " << fname);
    // That will have forked - we don't want to fork, so clear out all the chaff
    // in the new address space that's not in the kernel address space so we
    // have a clean slate.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    pProcess->getSpaceAllocator().clear();
    pProcess->getDynamicSpaceAllocator().clear();
    pProcess->getSpaceAllocator().free(
            pProcess->getAddressSpace()->getUserStart(),
            pProcess->getAddressSpace()->getUserReservedStart() - pProcess->getAddressSpace()->getUserStart());
    if(pProcess->getAddressSpace()->getDynamicStart())
    {
        pProcess->getDynamicSpaceAllocator().free(
            pProcess->getAddressSpace()->getDynamicStart(),
            pProcess->getAddressSpace()->getDynamicEnd() - pProcess->getAddressSpace()->getDynamicStart());
    }
    pProcess->getAddressSpace()->revertToKernelAddressSpace();

    DynamicLinker *pLinker = new DynamicLinker();
    pProcess->setLinker(pLinker);

    // Should we actually load this file, or request another program load the file?
    String interpreter("");
    if(pLinker->checkInterpreter(initProg, interpreter))
    {
        // Switch to the interpreter.
        initProg = VFS::instance().find(interpreter, pProcess->getCwd());
        if(!initProg)
        {
            FATAL("Unable to find init program interpreter '" << interpreter << "'!");
            return;
        }

        // Using the interpreter - don't worry about dynamic linking.
        delete pLinker;
        pLinker = 0;
        pProcess->setLinker(pLinker);
    }

    if (pLinker && !pLinker->loadProgram(initProg))
    {
        FATAL("Init program failed to load!");
    }

    for (int j = 0; j < 0x21000; j += 0x1000)
    {
        physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
        bool b = Processor::information().getVirtualAddressSpace().map(phys, reinterpret_cast<void*> (j+0x20000000), VirtualAddressSpace::Write);
        if (!b)
            WARNING("map() failed in init");
    }

    // Initialise the sigret and pthreads shizzle.
    pedigree_init_sigret();
    pedigree_init_pthreads();

    class RunInitEvent : public Event
    {
    public:
        RunInitEvent(uintptr_t addr) : Event(addr, true)
        {}
        size_t serialize(uint8_t *pBuffer)
        {return 0;}
        size_t getNumber() {return ~0UL;}
    };

    Elf *elf = 0;
    if(pLinker)
    {
        elf = pLinker->getProgramElf();
    }
    else
    {
        uintptr_t loadAddr = pProcess->getAddressSpace()->getDynamicLinkerAddress();
        MemoryMappedObject::Permissions perms = MemoryMappedObject::Read | MemoryMappedObject::Write | MemoryMappedObject::Exec;
        MemoryMappedObject *pMmFile = MemoryMapManager::instance().mapFile(initProg, loadAddr, initProg->getSize(), perms);
        if(!pMmFile)
        {
            FATAL("Couldn't memory map dynamic linker for init program");
        }

        // Create the ELF.
        /// \todo It'd be awesome if we could just pull out the entry address.
        elf = new Elf();
        elf->create(reinterpret_cast<uint8_t*>(loadAddr), initProg->getSize());
    }

    if(pLinker)
    {
        // Find the init function location, if it exists.
        uintptr_t initLoc = elf->getInitFunc();
        if (initLoc)
        {
            NOTICE("initLoc active: " << initLoc);

            RunInitEvent *ev = new RunInitEvent(initLoc);
            // Poke the initLoc so we know it's mapped in!
            volatile uintptr_t *vInitLoc = reinterpret_cast<volatile uintptr_t*> (initLoc);
            volatile uintptr_t tmp = * vInitLoc;
            *vInitLoc = tmp; // GCC can't ignore a write.
            asm volatile("" :::"memory"); // Memory barrier.
            Processor::information().getCurrentThread()->sendEvent(ev);
            // Yield, so the code gets run before we return.
            Scheduler::instance().yield();
        }
    }

    uintptr_t *argv_loc = reinterpret_cast<uintptr_t *>(0x20020000);
    memset(argv_loc, 0, PhysicalMemoryManager::instance().getPageSize());
    argv_loc[0] = reinterpret_cast<uintptr_t>(&argv_loc[2]);
    memcpy(&argv_loc[2], static_cast<const char *>(fname), fname.length());

    uintptr_t *env_loc = reinterpret_cast<uintptr_t *>(0x20020400);

    void *stack = Processor::information().getVirtualAddressSpace().allocateStack();

#if 0
    system_reset();
#else

    uintptr_t entryPoint = elf->getEntryPoint();
    if(!pLinker)
    {
        // Free up resources used in the metadata-only ELF object.
        delete elf;
    }

    // Alrighty - lets create a new thread for this program - -8 as PPC assumes
    // the previous stack frame is available...
    Thread *pThread = new Thread(
            pProcess,
            reinterpret_cast<Thread::ThreadStartFunc>(entryPoint),
            argv_loc /* parameter */,
            stack /* Stack */);
    pThread->detach();

    g_InitProgramLoaded.release();
#endif
}

#ifdef X86_COMMON
#define __MOD_DEPS "vfs", "posix", "partition", "linker", "network-stack", "users", "pedigree-c", "native"
#define __MOD_DEPS_OPT "ext2", "fat", "TUI", "vbe"
#elif PPC_COMMON
#define __MOD_DEPS "vfs", "ext2", "fat", "posix", "partition", "TUI", "linker", "network-stack", "users", "pedigree-c", "native"
#elif ARM_COMMON
#define __MOD_DEPS "vfs", "ext2", "fat", "posix", "partition", "linker", "network-stack", "users", "pedigree-c", "native"
#elif defined(HOSTED)
#define __MOD_DEPS "vfs", "ext2", "fat", "partition", "network-stack", "users", "pedigree-c", "native", "posix"
#endif
MODULE_INFO("init", &init, &destroy, __MOD_DEPS);
#ifdef __MOD_DEPS_OPT
MODULE_OPTIONAL_DEPENDS(__MOD_DEPS_OPT);
#endif
