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

#include <compiler.h>
#include <Log.h>
#include <Module.h>
#include <vfs/VFS.h>
#include <subsys/posix/PosixSubsystem.h>
#include <core/BootIO.h>
#include <linker/DynamicLinker.h>
#include <users/UserManager.h>

extern void pedigree_init_sigret();
extern void pedigree_init_pthreads();

static void error(const char *s)
{
    extern BootIO bootIO;
    static HugeStaticString str;
    str += s;
    str += "\n";
    bootIO.write(str, BootIO::Red, BootIO::Black);
    str.clear();
}

int init_stage2(void *param)
{
#if defined(HOSTED) && defined(HAS_ADDRESS_SANITIZER)
    extern void system_reset();
    NOTICE("Note: ASAN build, so triggering a restart now.");
    system_reset();
    return;
#endif

    // Load initial program.
    String fname = String("root»/applications/init");
    File* initProg = VFS::instance().find(fname);
    if (!initProg)
    {
        error("Loading init program FAILED (root»/applications/init not found).");
        return 0;
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
            error("Interpreter for init program could not be found.");
            return 0;
        }

        // Using the interpreter - don't worry about dynamic linking.
        delete pLinker;
        pLinker = 0;
        pProcess->setLinker(pLinker);
    }

    if (pLinker && !pLinker->loadProgram(initProg))
    {
        error("The init program could not be loaded.");
        return 0;
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

    uintptr_t entryPoint = 0;

    Elf *elf = 0;
    if(pLinker)
    {
        elf = pLinker->getProgramElf();
        entryPoint = elf->getEntryPoint();
    }
    else
    {
        uintptr_t loadAddr = pProcess->getAddressSpace()->getDynamicLinkerAddress();
        MemoryMappedObject::Permissions perms = MemoryMappedObject::Read | MemoryMappedObject::Write | MemoryMappedObject::Exec;
        MemoryMappedObject *pMmFile = MemoryMapManager::instance().mapFile(initProg, loadAddr, initProg->getSize(), perms);
        if(!pMmFile)
        {
            error("Memory for the dynamic linker could not be allocated.");
            return 0;
        }

        Elf::extractEntryPoint(reinterpret_cast<uint8_t *>(loadAddr), initProg->getSize(), entryPoint);
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

    // can we get some space for the argv loc
    uintptr_t argv_loc = 0;
    if (pProcess->getAddressSpace()->getDynamicStart())
    {
        pProcess->getDynamicSpaceAllocator().allocate(PhysicalMemoryManager::instance().getPageSize(), argv_loc);
    }
    if (!argv_loc)
    {
        pProcess->getSpaceAllocator().allocate(PhysicalMemoryManager::instance().getPageSize(), argv_loc);
        if (!argv_loc)
        {
            FATAL("init could not find space in the address space for argv!");
        }
    }

    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    Processor::information().getVirtualAddressSpace().map(phys, reinterpret_cast<void*> (argv_loc), VirtualAddressSpace::Write);

    uintptr_t *argv = reinterpret_cast<uintptr_t*>(argv_loc);
    ByteSet(argv, 0, PhysicalMemoryManager::instance().getPageSize());
    argv[0] = reinterpret_cast<uintptr_t>(&argv[2]);
    MemoryCopy(&argv[2], static_cast<const char *>(fname), fname.length());

    void *stack = Processor::information().getVirtualAddressSpace().allocateStack();

    Processor::setInterrupts(true);
    pProcess->recordTime(true);

    // Alrighty - lets create a new thread for this program - -8 as PPC assumes
    // the previous stack frame is available...
    NOTICE("spinning up main thread for init now");
    Thread *pThread = new Thread(
            pProcess,
            reinterpret_cast<Thread::ThreadStartFunc>(entryPoint),
            argv,
            stack);
    pThread->detach();

    return 0;
}

static bool init()
{
#ifdef THREADS
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

    Thread *pThread = new Thread(pProcess, init_stage2, 0);
    pThread->detach();
#endif

    return true;
}

static void destroy()
{
}

#if defined(X86_COMMON)
#define __MOD_DEPS "vfs", "posix", "linker", "users"
#define __MOD_DEPS_OPT "gfx-deps", "mountroot"
#else
#define __MOD_DEPS "vfs", "posix", "linker", "users"
#define __MOD_DEPS_OPT "mountroot"
#endif
MODULE_INFO("init", &init, &destroy, __MOD_DEPS);
#ifdef __MOD_DEPS_OPT
MODULE_OPTIONAL_DEPENDS(__MOD_DEPS_OPT);
#endif
