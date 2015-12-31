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

#include "file-syscalls.h"
#include "system-syscalls.h"
#include "pipe-syscalls.h"
#include "signal-syscalls.h"
#include "pthread-syscalls.h"
#include "posixSyscallNumbers.h"
#include <syscallError.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/state.h>
#include <process/Process.h>
#include <process/Thread.h>
#include <process/Scheduler.h>
#include <Log.h>
#include <linker/Elf.h>
#include <linker/KernelElf.h>
#include <vfs/File.h>
#include <vfs/Symlink.h>
#include <vfs/VFS.h>
#include <panic.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/StackFrame.h>
#include <linker/DynamicLinker.h>
#include <utilities/String.h>
#include <utilities/Vector.h>

#define MACHINE_FORWARD_DECL_ONLY
#include <machine/Machine.h>
#include <machine/Timer.h>

#include <Subsystem.h>
#include <PosixSubsystem.h>
#include <PosixProcess.h>

#include <users/UserManager.h>
#include <console/Console.h>

//
// Syscalls pertaining to system operations.
//

#define GET_CWD() (Processor::information().getCurrentThread()->getParent()->getCwd())

/// Clears a string array.
static void clear_string_array(Vector<String*> &rArray)
{
    for (Vector<String*>::Iterator it = rArray.begin();
            it != rArray.end();
            it++)
    {
        String *pStr = *it;
        delete pStr;
    }

    rArray.clear();
}
/// Saves a char** array in the Vector of String*s given.
static size_t save_string_array(const char **array, Vector<String*> &rArray)
{
    size_t result = 0;
    while (*array)
    {
        String *pStr = new String(*array);
        rArray.pushBack(pStr);
        array++;

        result += pStr->length() + 1;
    }

    return result;
}
/// Creates a char** array, properly null-terminated, from the Vector of String*s given, at the location "arrayLoc",
/// returning the end of the char** array created in arrayEndLoc and the start as the function return value.
static char **load_string_array(Vector<String*> &rArray, uintptr_t arrayLoc, uintptr_t &arrayEndLoc)
{
    char **pMasterArray = reinterpret_cast<char**> (arrayLoc);

    char *pPtr = reinterpret_cast<char*> (arrayLoc + sizeof(char*) * (rArray.count()+1) );
    int i = 0;
    for (Vector<String*>::Iterator it = rArray.begin();
            it != rArray.end();
            it++)
    {
        String *pStr = *it;

        strcpy(pPtr, *pStr);
        pPtr[pStr->length()] = '\0'; // Ensure NULL-termination.

        pMasterArray[i] = pPtr;

        pPtr += pStr->length()+1;
        i++;

        delete pStr;
    }

    pMasterArray[i] = 0; // Null terminate.
    arrayEndLoc = reinterpret_cast<uintptr_t> (pPtr);

    return pMasterArray;
}

long posix_sbrk(int delta)
{
    SC_NOTICE("sbrk(" << delta << ")");

    long ret = reinterpret_cast<long>(
                   Processor::information().getVirtualAddressSpace().expandHeap (delta, VirtualAddressSpace::Write));
    SC_NOTICE("    -> " << ret);
    if (ret == 0)
    {
        SYSCALL_ERROR(OutOfMemory);
        return -1;
    }
    else
        return ret;
}

int posix_fork(SyscallState &state)
{
    SC_NOTICE("fork()");

    Processor::setInterrupts(false);

    // Inhibit signals to the parent
    for(int sig = 0; sig < 32; sig++)
        Processor::information().getCurrentThread()->inhibitEvent(sig, true);

    // Create a new process.
    Process *pParentProcess = Processor::information().getCurrentThread()->getParent();
    PosixProcess *pProcess = new PosixProcess(pParentProcess);
    if (!pProcess)
    {
        SYSCALL_ERROR(OutOfMemory);
        return -1;
    }

    PosixSubsystem *pParentSubsystem = reinterpret_cast<PosixSubsystem*>(pParentProcess->getSubsystem());
    PosixSubsystem *pSubsystem = new PosixSubsystem(*pParentSubsystem);
    if (!pSubsystem || !pParentSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");

        if(pSubsystem)
            delete pSubsystem;
        if(pParentSubsystem)
            delete pParentSubsystem;
        delete pProcess;

        SYSCALL_ERROR(OutOfMemory);

        // Allow signals again, something went wrong
        for(int sig = 0; sig < 32; sig++)
            Processor::information().getCurrentThread()->inhibitEvent(sig, false);
        return -1;
    }
    pProcess->setSubsystem(pSubsystem);
    pSubsystem->setProcess(pProcess);

    // Copy POSIX Process Group information if needed
    if(pParentProcess->getType() == Process::Posix)
    {
        PosixProcess *p = static_cast<PosixProcess*>(pParentProcess);
        pProcess->setProcessGroup(p->getProcessGroup());

        // Do not adopt leadership status.
        if(p->getGroupMembership() == PosixProcess::Leader)
        {
            SC_NOTICE("fork parent was a group leader.");
            pProcess->setGroupMembership(PosixProcess::Member);
        }
        else
        {
            SC_NOTICE("fork parent had status " << static_cast<int>(p->getGroupMembership()) << "...");
            pProcess->setGroupMembership(p->getGroupMembership());
        }
    }

    // Register with the dynamic linker.
    DynamicLinker *oldLinker = pProcess->getLinker();
    if(oldLinker)
    {
        DynamicLinker *newLinker = new DynamicLinker(*oldLinker);
        pProcess->setLinker(newLinker);
    }

    MemoryMapManager::instance().clone(pProcess);

    // Copy the file descriptors from the parent
    pSubsystem->copyDescriptors(pParentSubsystem);

    // Child returns 0.
    state.setSyscallReturnValue(0);

    // Allow signals to the parent again
    for(int sig = 0; sig < 32; sig++)
        Processor::information().getCurrentThread()->inhibitEvent(sig, false);

    // Create a new thread for the new process.
    Thread *pThread = new Thread(pProcess, state);
    pThread->detach();

    // Fix up the main thread in the child.
    pedigree_copy_posix_thread(
        Processor::information().getCurrentThread(), pParentSubsystem,
        pThread, pSubsystem);

    // Kick off the new thread immediately.
    Scheduler::instance().yield();

    // Parent returns child ID.
    return pProcess->getId();
}

int posix_execve(const char *name, const char **argv, const char **env, SyscallState &state)
{
    /// \todo Check argv/env??
    if(!PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(name), PATH_MAX, PosixSubsystem::SafeRead))
    {
        SC_NOTICE("execve -> invalid address");
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    SC_NOTICE("execve(\"" << name << "\")");

    size_t pageSz = PhysicalMemoryManager::getPageSize();

    String myArgv;
    int i = 0;
    while (argv[i])
    {
        myArgv += String(argv[i]);
        myArgv += " ";
        i++;
    }
    SC_NOTICE("  {" << myArgv << "}");

    if (argv == 0 || env == 0)
    {
        SYSCALL_ERROR(ExecFormatError);
        return -1;
    }

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Grab the thread we're going to return into - need to tweak it.
    Thread *pThread = pProcess->getThread(0);

    // Ensure we only have one thread running (us).
    if (pProcess->getNumThreads() > 1)
    {
        SYSCALL_ERROR(ExecFormatError);
        return -1;
    }

    String toLoad;
    normalisePath(toLoad, name);

    // Attempt to find the file, first!
    File *pActualFile = 0;
    File* file = pActualFile = VFS::instance().find(toLoad, Processor::information().getCurrentThread()->getParent()->getCwd());
    if (!file)
    {
        // Error - not found.
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }

    while (file->isSymlink())
        file = Symlink::fromFile(file)->followLink();

    if (!file)
    {
        // Not found after symlink traversal.
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }

    if (file->isDirectory())
    {
        // Error - is directory.
        SYSCALL_ERROR(IsADirectory);
        return -1;
    }

    // The argv and environment for the new process
    Vector<String*> savedArgv, savedEnv;

    /// \todo This could probably be cleaned up a little.

    // Try and read the shebang, if any
    String theShebang;
    static char tmpBuff[128 + 1];
    file->read(0, 128, reinterpret_cast<uintptr_t>(tmpBuff));
    tmpBuff[128] = '\0';

    List<String*> additionalArgv;

    if (!strncmp(tmpBuff, "#!", 2))
    {
        // We have a shebang, so grab the command.

        // Scan the found string for a newline. If we don't get a newline, the shebang was over 128 bytes and we error out.
        bool found = false;
        for (size_t i = 0; i < 128; i++)
        {
            if (tmpBuff[i] == '\n')
            {
                found = true;
                tmpBuff[i] = '\0';
                break;
            }
        }

        if (!found)
        {
            // Parameter error.
            SYSCALL_ERROR(InvalidArgument);
            return -1;
        }

        // Shebang loaded, tokenise.
        String str(&tmpBuff[2]); // Skip #!
        additionalArgv = str.tokenise(' ');

        if(additionalArgv.begin() == additionalArgv.end()) {
            NOTICE("Invalid shebang");
            SYSCALL_ERROR(ExecFormatError);
            return -1;
        }

        String *newFname = *additionalArgv.begin();

        // Prepend the tokenized shebang line argv
        // argv will look like: interpreter-path additional-shebang-options script-name old-argv
        for (List<String*>::Iterator it = additionalArgv.begin();
             it != additionalArgv.end();
             it++)
        {
            savedArgv.pushBack(*it);
        }

        // Replace the old argv[0] with the script name, this is what Linux does
        // ### is it safe to write to argv?
        argv[0] = name;

        name = static_cast<const char*>(*newFname);

        // And reload the file, now that we're loading a new application
        NOTICE("New name: " << *newFname << "...");
        file = VFS::instance().find(*newFname, Processor::information().getCurrentThread()->getParent()->getCwd());
        if (!file)
        {
            // Error - not found.
            SYSCALL_ERROR(DoesNotExist);
            return -1;
        }
    }

    DynamicLinker *pOldLinker = pProcess->getLinker();
    DynamicLinker *pLinker = new DynamicLinker();

    // Should we actually load this file, or request another program load the file?
    String interpreter("");
    if(pLinker->checkInterpreter(file, interpreter))
    {
        // Handle interpreter on root alias while we're on a different alias.
        File *root = VFS::instance().find(String("root»/"));

        // Switch to the interpreter.
        // argv can stay the same, as the interpreter will pass it on directly.
        file = VFS::instance().find(interpreter, Processor::information().getCurrentThread()->getParent()->getCwd());
        if ((!file) && root)
            file = VFS::instance().find(interpreter, root);
        if (!file)
        {
            ERROR("execve: interpreter '" << interpreter << "' not found.");
            SYSCALL_ERROR(DoesNotExist);
            delete pLinker;
            return -1;
        }

        // Welp, wipe out the old linker, don't leave crufty mmaps lying around.
        // We are changing which file to load - get a new linker for that.
        delete pLinker;
        pLinker = 0;
        pProcess->setLinker(pLinker);
    }
    else
    {
        /// \todo Assume the interpreter is libload.so.
        WARNING("execve: no interpreter for " << name << ".");
        delete pLinker;
        SYSCALL_ERROR(ExecFormatError);
        return -1;
    }

    // Can we load the new image? Check before we clean out the last ELF image...
    if(pLinker && !pLinker->checkDependencies(file))
    {
        delete pLinker;
        SYSCALL_ERROR(ExecFormatError);
        return -1;
    }

    // Now that dependencies are definitely available and the program will
    // actually load, we can set up the Process object
    pProcess->description() = String(name);

    /// \todo Write pActualFile->getFullPath() into argv[0]
    // Make sure the dynamic linker loads the correct program.
    String actualPath = pActualFile->getFullPath();
    savedArgv.pushFront(&actualPath);

    // Save the argv and env lists so they aren't destroyed when we overwrite the address space.
    size_t argv_len = save_string_array(argv, savedArgv);
    size_t env_len = save_string_array(env, savedEnv);

    // Inhibit all signals from coming in while we trash the address space...
    for(int sig = 0; sig < 32; sig++)
        Processor::information().getCurrentThread()->inhibitEvent(sig, true);

    // Wipe out old address space.
    MemoryMapManager::instance().unmapAll();
    pProcess->getAddressSpace()->revertToKernelAddressSpace();

    // Prepare the new address space regions.
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

    // Reset tracking.
    pProcess->resetCounts();

    // Reset TLS base.
    pThread->resetTlsBase();

    if(pLinker)
    {
        // Set the new linker now before we loadProgram, else we could trap and
        // have a linker mismatch.
        pProcess->setLinker(pLinker);

        if (!pLinker->loadProgram(file))
        {
            pProcess->setLinker(pOldLinker);
            delete pLinker;
            SYSCALL_ERROR(ExecFormatError);

            // Allow signals again, even though the address space is in a completely undefined state now
            for(int sig = 0; sig < 32; sig++)
                pProcess->getThread(0)->inhibitEvent(sig, false);
            return -1;
        }
        delete pOldLinker;
    }

    // Close all FD_CLOEXEC descriptors.
    pSubsystem->freeMultipleFds(true);

    // Clean up the thread now.
    /// \todo This doesn't actually free any stacks.
    while (pThread->getStateLevel())
        pThread->popState();

    // Create a new stack.
    uintptr_t newStack = reinterpret_cast<uintptr_t>(Processor::information().getVirtualAddressSpace().allocateStack());
    /// \todo Free previous stack, or actually use it??

    // Allocate space for argv and the new environment.
    uintptr_t argvAddress = 0;
    size_t totalArgvSpace = argv_len + env_len + (sizeof(void *) * (savedArgv.size() + savedEnv.size()));

    // Align to a page boundary as needed.
    if (totalArgvSpace & (pageSz - 1))
    {
        totalArgvSpace = (totalArgvSpace + pageSz) & ~(pageSz - 1);
    }

    // Get space now.
    if (pProcess->getAddressSpace()->getDynamicStart())
        pProcess->getDynamicSpaceAllocator().allocate(totalArgvSpace, argvAddress);
    if (!argvAddress)
        pProcess->getSpaceAllocator().allocate(totalArgvSpace, argvAddress);

    // Map in argv/env
    for (uintptr_t j = argvAddress; j < (argvAddress + totalArgvSpace); j += pageSz)
    {
        void *pVirt = reinterpret_cast<void*> (j);
        physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
        bool b = Processor::information().getVirtualAddressSpace().map(phys,
                                                                       pVirt,
                                                                       VirtualAddressSpace::Write);
        if (!b)
            WARNING("map() failed in execve when mapping argv/env");
    }

    // Load the saved argv and env into this address space now.
    uintptr_t location = argvAddress;
    argv = const_cast<const char**> (load_string_array(savedArgv, location, location));
    env  = const_cast<const char**> (load_string_array(savedEnv , location, location));

    // Wipe out the argv and env now.
    clear_string_array(savedArgv);
    clear_string_array(savedEnv);

    uintptr_t entryPoint = 0;

    Elf *elf = 0;
    if(pLinker)
    {
        elf = pProcess->getLinker()->getProgramElf();
        entryPoint = elf->getEntryPoint();
    }
    else
    {
        // Memory map the interpreter and create an Elf object for it.
        // The memory map is defined as unshared, but because we map files
        // with CoW, most of this mapping ends up shared across all address
        // spaces. This means we don't have to be smart about loading the ELF.
        uintptr_t loadAddr = pProcess->getAddressSpace()->getDynamicLinkerAddress();
        NOTICE("Mapping dynamic linker '" << file->getName() << "' at " << loadAddr << "...");
        MemoryMappedObject::Permissions perms = MemoryMappedObject::Read | MemoryMappedObject::Write | MemoryMappedObject::Exec;
        MemoryMappedObject *pMmFile = MemoryMapManager::instance().mapFile(file, loadAddr, file->getSize(), perms);
        if(!pMmFile)
        {
            ERROR("execve: couldn't memory map dynamic linker");
            SYSCALL_ERROR(ExecFormatError);
            return -1;
        }

        Elf::extractEntryPoint(reinterpret_cast<uint8_t*>(loadAddr), file->getSize(), entryPoint);
    }

    // JAMESM: I don't think the sigret code actually needs to be called from userspace. Here should do just fine, no?

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

    // Don't run init if we don't have a linker (loaded image needs to relocate itself and call init)
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

    /// \todo Genericize this somehow - "pState.setScratchRegisters(state)"?
#ifdef PPC_COMMON
    state.m_R6 = pState.m_R6;
    state.m_R7 = pState.m_R7;
    state.m_R8 = pState.m_R8;
#endif

    // Before we enter the new address space, wipe out the scheduler state.
    // This is a new scheduling entity.
    SchedulerState s;
    memset(&s, 0, sizeof(s));
    pThread->state() = s;

    // Allow signals again now that everything's loaded
    for(int sig = 0; sig < 32; sig++)
        Processor::information().getCurrentThread()->inhibitEvent(sig, false);

    // Jump to the new process.
    Processor::setInterrupts(true);
    pProcess->recordTime(true);
    Processor::jumpUser(0, entryPoint, newStack,
        reinterpret_cast<uintptr_t>(argv), reinterpret_cast<uintptr_t>(env));

    // Never reaches this point.
    return 0;
}

/**
 * Class intended to be used for RAII to clean up waitpid state on exit.
 */
class WaitCleanup
{
    public:
        WaitCleanup(List<Process*> *cleanupList, Semaphore *lock) :
            m_List(cleanupList), m_Lock(lock), m_pTerminated(0)
        {}

        /**
         * Call this with the process that terminated most recently, which
         * is necessary because otherwise upon exit from waitpid() we attempt
         * to access the (deleted) Process object, which is not safe.
         */
        void terminated(Process *pProcess)
        {
            m_pTerminated = pProcess;
            pProcess->removeWaiter(m_Lock);
        }

        ~WaitCleanup()
        {
            for(List<Process*>::Iterator it = m_List->begin();
                it != m_List->end();
                ++it)
            {
                if((*it) == m_pTerminated)
                    continue;

                (*it)->removeWaiter(m_Lock);
            }
        }

    private:

        List<Process *> *m_List;
        Semaphore *m_Lock;
        Process *m_pTerminated;
};

int posix_waitpid(int pid, int *status, int options)
{
    if(status && !PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(status), sizeof(int), PosixSubsystem::SafeWrite))
    {
        SC_NOTICE("waitpid -> invalid address");
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    SC_NOTICE("waitpid(" << pid << ", " << options << ")");

    // Find the set of processes to check.
    List<Process *> processList;

    // Our lock, which we will assign to each process (assuming WNOHANG is not set).
    Semaphore waitLock(0);

    // RAII object to clean up when we return (instead of goto or other ugliness).
    WaitCleanup cleanup(&processList, &waitLock);

    // Metadata about the calling process.
    PosixProcess *pThisProcess = static_cast<PosixProcess *>(Processor::information().getCurrentThread()->getParent());
    ProcessGroup *pThisGroup = pThisProcess->getProcessGroup();

    // Check for the process(es) we need to check for.
    size_t i = 0;
    bool bBlock = (options & WNOHANG) != WNOHANG;
    for(; i < Scheduler::instance().getNumProcesses(); ++i)
    {
        Process *pProcess = Scheduler::instance().getProcess(i);
        if(pProcess == pThisProcess)
            continue; // Don't wait for ourselves.

        if (pProcess->getState() == Process::Reaped)
            continue; // Reaped but not yet destroyed.

        if ((pid <= 0) && (pProcess->getType() == Process::Posix))
        {
            PosixProcess *pPosixProcess = static_cast<PosixProcess *>(pProcess);
            ProcessGroup *pGroup = pPosixProcess->getProcessGroup();
            if (pid == 0)
            {
                // Any process in the same process group as the caller.
                if (!(pGroup && pThisGroup))
                    continue;
                if(pGroup->processGroupId != pThisGroup->processGroupId)
                    continue;
            }
            else if (pid == -1)
            {
                // Wait for any child.
                if(pProcess->getParent() != pThisProcess)
                    continue;
            }
            else if(pGroup && (pGroup->processGroupId != (pid * -1)))
            {
                // Absolute group ID reference
                continue;
            }
        }
        else if ((pid > 0) && (static_cast<int>(pProcess->getId()) != pid))
            continue;
        else if (pProcess->getType() != Process::Posix)
            continue;

        // Okay, the process is good.
        processList.pushBack(pProcess);

        // If not WNOHANG, subscribe our lock to this process' state changes.
        // If the process is in the process of terminating, we can add our
        // lock and hope for the best.
        if(bBlock || (pProcess->getState() == Process::Terminating))
        {
            SC_NOTICE("  -> adding our wait lock to process " << pProcess->getId());
            pProcess->addWaiter(&waitLock);
            bBlock = true;
        }
    }

    // No children?
    if(processList.count() == 0)
    {
        SYSCALL_ERROR(NoChildren);
        SC_NOTICE("  -> no children");
        return -1;
    }

    // Main wait loop.
    while(1)
    {
        // Check each process for state.
        for(List<Process *>::Iterator it = processList.begin();
            it != processList.end();
            ++it)
        {
            Process *pProcess = *it;
            pid = pProcess->getId();

            // Zombie?
            if (pProcess->getState() == Process::Terminated)
            {
                if (status)
                    *status = pProcess->getExitStatus();

                // Delete the process; it's been reaped good and proper.
                SC_NOTICE("waitpid: " << pid << " reaped [" << pProcess->getExitStatus() << "]");
                cleanup.terminated(pProcess);
                if (pProcess->waiterCount() < 1)
                    delete pProcess;
                else
                    pProcess->reap();
                return pid;
            }
            // Suspended (and WUNTRACED)?
            else if((options & 2) && pProcess->hasSuspended())
            {
                if(status)
                    *status = pProcess->getExitStatus();

                SC_NOTICE("waitpid: " << pid << " suspended.");
                return pid;
            }
            // Continued (and WCONTINUED)?
            else if((options & 4) && pProcess->hasResumed())
            {
                if(status)
                    *status = pProcess->getExitStatus();

                SC_NOTICE("waitpid: " << pid << " resumed.");
                return pid;
            }
        }

        // Don't wait for any processes to report status if we are not meant
        // to be blocking.
        if(!bBlock)
        {
            return 0;
        }

        // Wait for processes to report in.
        waitLock.acquire();

        // We can get woken up by our process dying. Handle that here.
        if (Processor::information().getCurrentThread()->getUnwindState() == Thread::Exit)
        {
            SC_NOTICE("waitpid: unwind state means exit");
            return -1;
        }

        // We get notified by processes just before they change state.
        // Make sure they are scheduled into that state by yielding.
        Scheduler::instance().yield();
    }

    return -1;
}

int posix_exit(int code)
{
    SC_NOTICE("exit(" << Dec << (code&0xFF) << Hex << ")");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());

    pSubsystem->exit(code);

    // Should NEVER get here.
    FATAL("Subsystem::exit() returned in posix_exit");
}

int posix_getpid()
{
    SC_NOTICE("getpid");
    
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    return pProcess->getId();
}

int posix_getppid()
{
    SC_NOTICE("getppid");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    if (!pProcess->getParent())
        return 0;
    return pProcess->getParent()->getId();
}

int posix_gettimeofday(timeval *tv, struct timezone *tz)
{
    if(!PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(tv), sizeof(timeval), PosixSubsystem::SafeWrite))
    {
        SC_NOTICE("gettimeofday -> invalid address");
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    SC_NOTICE("gettimeofday");
    
    Timer *pTimer = Machine::instance().getTimer();

    // UNIX timestamp + remaining time portion, in microseconds.
    tv->tv_sec = pTimer->getUnixTimestamp();
    tv->tv_usec = pTimer->getNanosecond() / 1000U;

    return 0;
}

clock_t posix_times(struct tms *tm)
{
    if(!PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(tm), sizeof(struct tms), PosixSubsystem::SafeWrite))
    {
        SC_NOTICE("posix_times -> invalid address");
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    SC_NOTICE("times");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();

    memset(tm, 0, sizeof(struct tms));
    tm->tms_utime = pProcess->getUserTime();
    tm->tms_stime = pProcess->getKernelTime();

    NOTICE("times: u=" << pProcess->getUserTime() << ", s=" << pProcess->getKernelTime());

    return Time::getTimeNanoseconds() - pProcess->getStartTime();
}

int posix_getrusage(int who, struct rusage *r)
{
    SC_NOTICE("getrusage who=" << who);

    if(!PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(r), sizeof(struct rusage), PosixSubsystem::SafeWrite))
    {
        SC_NOTICE("posix_getrusage -> invalid address");
        SYSCALL_ERROR(BadAddress);
        return -1;
    }

    if (who != RUSAGE_SELF)
    {
        SC_NOTICE("posix_getrusage -> non-RUSAGE_SELF not supported");
        SYSCALL_ERROR(InvalidArgument);
        memset(r, 0, sizeof(struct rusage));
        return -1;
    }

    Process *pProcess = Processor::information().getCurrentThread()->getParent();

    Time::Timestamp user = pProcess->getUserTime();
    Time::Timestamp kernel = pProcess->getKernelTime();

    memset(r, 0, sizeof(struct rusage));
    r->ru_utime.tv_sec = user / Time::Multiplier::SECOND;
    r->ru_utime.tv_usec = (user % Time::Multiplier::SECOND) / Time::Multiplier::MICROSECOND;
    r->ru_stime.tv_sec = kernel / Time::Multiplier::SECOND;
    r->ru_stime.tv_usec = (kernel % Time::Multiplier::SECOND) / Time::Multiplier::MICROSECOND;

    return 0;
}

char *store_str_to(char *str, char *strend, String s)
{
    int i = 0;
    while (s[i] && str != strend)
        *str++ = s[i++];
    *str++ = '\0';

    return str;
}

int posix_getpwent(passwd *pw, int n, char *str)
{
    /// \todo 'str' is not very nice here, can we do this better?
    if(!PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(pw), sizeof(passwd), PosixSubsystem::SafeWrite))
    {
        SC_NOTICE("getpwent -> invalid address");
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    SC_NOTICE("getpwent(" << Dec << n << Hex << ")");

    // Grab the given user.
    User *pUser = UserManager::instance().getUser(n);
    if (!pUser) return -1;

    char *strend = str + 256; // If we get here, we've gone off the end of str.

    pw->pw_name = str;
    str = store_str_to(str, strend, pUser->getUsername());

    pw->pw_passwd = str;
    *str++ = '\0';

    pw->pw_uid = pUser->getId();
    pw->pw_gid = pUser->getDefaultGroup()->getId();
    pw->pw_comment = str;
    str = store_str_to(str, strend, pUser->getFullName());

    pw->pw_gecos = str;
    *str++ = '\0';
    pw->pw_dir = str;
    str = store_str_to(str, strend, pUser->getHome());

    pw->pw_shell = str;
    store_str_to(str, strend, pUser->getShell());

    return 0;
}

int posix_getpwnam(passwd *pw, const char *name, char *str)
{
    /// \todo Again, str is not very nice here.
    if(!(PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(pw), sizeof(passwd), PosixSubsystem::SafeWrite) &&
        PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(name), PATH_MAX, PosixSubsystem::SafeRead)))
    {
        SC_NOTICE("readdir -> invalid address");
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    SC_NOTICE("getpwname(" << name << ")");
    
    // Grab the given user.
    User *pUser = UserManager::instance().getUser(String(name));
    if (!pUser) return -1;

    char *strend = str + 256; // If we get here, we've gone off the end of str.

    pw->pw_name = str;
    str = store_str_to(str, strend, pUser->getUsername());

    pw->pw_passwd = str;
    *str++ = '\0';

    pw->pw_uid = pUser->getId();
    pw->pw_gid = pUser->getDefaultGroup()->getId();
    pw->pw_comment = str;
    str = store_str_to(str, strend, pUser->getFullName());

    pw->pw_gecos = str;
    *str++ = '\0';

    pw->pw_dir = str;
    str = store_str_to(str, strend, pUser->getHome());

    pw->pw_shell = str;
    store_str_to(str, strend, pUser->getShell());

    return 0;
}

uid_t posix_getuid()
{
    SC_NOTICE("getuid() -> " << Dec << Processor::information().getCurrentThread()->getParent()->getUser()->getId());
    return Processor::information().getCurrentThread()->getParent()->getUser()->getId();
}

gid_t posix_getgid()
{
    SC_NOTICE("getgid() -> " << Dec << Processor::information().getCurrentThread()->getParent()->getGroup()->getId());
    return Processor::information().getCurrentThread()->getParent()->getGroup()->getId();
}

uid_t posix_geteuid()
{
    SC_NOTICE("geteuid() -> " << Dec << Processor::information().getCurrentThread()->getParent()->getEffectiveUser()->getId());
    return Processor::information().getCurrentThread()->getParent()->getEffectiveUser()->getId();
}

gid_t posix_getegid()
{
    SC_NOTICE("getegid() -> " << Dec << Processor::information().getCurrentThread()->getParent()->getEffectiveGroup()->getId());
    return Processor::information().getCurrentThread()->getParent()->getEffectiveGroup()->getId();
}

int posix_setuid(uid_t uid)
{
    SC_NOTICE("setuid(" << uid << ")");
    
    /// \todo Missing "set user"
    User *user = UserManager::instance().getUser(uid);
    if(!user)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }
    
    Processor::information().getCurrentThread()->getParent()->setUser(user);
    Processor::information().getCurrentThread()->getParent()->setEffectiveUser(user);
    
    return 0;
}

int posix_setgid(gid_t gid)
{
    SC_NOTICE("setgid(" << gid << ")");
    
    /// \todo Missing "set user"
    Group *group= UserManager::instance().getGroup(gid);
    if(!group)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }
    
    Processor::information().getCurrentThread()->getParent()->setGroup(group);
    Processor::information().getCurrentThread()->getParent()->setEffectiveGroup(group);
    
    return 0;
}

int posix_seteuid(uid_t euid)
{
    SC_NOTICE("seteuid(" << euid << ")");
    
    /// \todo Missing "set user"
    User *user = UserManager::instance().getUser(euid);
    if(!user)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }
    
    Processor::information().getCurrentThread()->getParent()->setEffectiveUser(user);
    
    return 0;
}

int posix_setegid(gid_t egid)
{
    SC_NOTICE("setegid(" << egid << ")");
    
    /// \todo Missing "set user"
    Group *group= UserManager::instance().getGroup(egid);
    if(!group)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }
    
    Processor::information().getCurrentThread()->getParent()->setEffectiveGroup(group);
    
    return 0;
}


int pedigree_login(int uid, const char *password)
{
    if(!PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(password), PATH_MAX, PosixSubsystem::SafeRead))
    {
        SC_NOTICE("pedigree_login -> invalid address");
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Grab the given user.
    User *pUser = UserManager::instance().getUser(uid);
    if (!pUser) return -1;

    if (pUser->login(String(password)))
        return 0;
    else
        return -1;
}

int posix_setsid()
{
    SC_NOTICE("setsid");

    // Not a POSIX process
    Process *pStockProcess = Processor::information().getCurrentThread()->getParent();
    if(pStockProcess->getType() != Process::Posix)
    {
        ERROR("setsid called on something not a POSIX process");
        return -1;
    }

    PosixProcess *pProcess = static_cast<PosixProcess *>(pStockProcess);

    // Already in a group?
    PosixProcess::Membership myMembership = pProcess->getGroupMembership();
    if(myMembership != PosixProcess::NoGroup)
    {
        // If we don't actually have a group, something's gone wrong
        if(!pProcess->getProcessGroup())
            FATAL("Process' is apparently a member of a group, but its group pointer is invalid.");

        // Are we the group leader of that other group?
        if(myMembership == PosixProcess::Leader)
        {
            SC_NOTICE("setsid() called while the leader of another group");
            SYSCALL_ERROR(PermissionDenied);
            return -1;
        }
        else
        {
            SC_NOTICE("setsid() called while a member of another group");
        }
    }

    // Delete the old group, if any
    ProcessGroup *pGroup = pProcess->getProcessGroup();
    if(pGroup)
    {
        pProcess->setProcessGroup(0);

        /// \todo Remove us from the list
        /// \todo Remove others from the list!?
        if(pGroup->Members.count() <= 1) // Us or nothing
            delete pGroup;
    }

    // Create the new session.
    PosixSession *pNewSession = new PosixSession();
    pNewSession->Leader = pProcess;
    pProcess->setSession(pNewSession);

    // Create a new process group and join it.
    ProcessGroup *pNewGroup = new ProcessGroup;
    pNewGroup->processGroupId = pProcess->getId();
    pNewGroup->Leader = pProcess;
    pNewGroup->Members.clear();

    // We're now a group leader - we got promoted!
    pProcess->setProcessGroup(pNewGroup);
    pProcess->setGroupMembership(PosixProcess::Leader);

    // Remove controlling terminal.
    pProcess->setCtty(0);

    SC_NOTICE("setsid: now part of a group [id=" << pNewGroup->processGroupId << "]!");

    // Success!
    return pNewGroup->processGroupId;
}

int posix_setpgid(int pid_, int pgid)
{
    size_t pid = pid_;
    SC_NOTICE("setpgid(" << pid << ", " << pgid << ")");

    // Handle invalid group ID
    if(pgid < 0)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    Process *pBaseProcess = Processor::information().getCurrentThread()->getParent();
    if(pBaseProcess->getType() != Process::Posix)
    {
        SC_NOTICE("  -> not a posix process");
        return -1;
    }

    // Are we already a leader of a session?
    PosixProcess *pProcess = static_cast<PosixProcess *>(pBaseProcess);
    ProcessGroup *pGroup = pProcess->getProcessGroup();
    PosixSession *pSession = pProcess->getSession();

    // Handle zero PID and PGID.
    if(!pid)
    {
        pid = pProcess->getId();
    }
    if(!pgid)
    {
        pgid = pid;
    }

    // Is this us or a child of us?
    /// \todo pid == child, but child not in this session = EPERM
    if(pid != pProcess->getId())
    {
        /// \todo We actually have no way of finding children of a process sanely.
        SYSCALL_ERROR(PermissionDenied);
        return 0;
    }

    if(pGroup && (pGroup->processGroupId == pgid))
    {
        // Already a member.
        return 0;
    }

    if(pSession && (pSession->Leader == pProcess))
    {
        // Already a session leader.
        SYSCALL_ERROR(PermissionDenied);
        return 0;
    }

    // Does the process group exist?
    Process *check = 0;
    for(size_t i = 0; i < Scheduler::instance().getNumProcesses(); ++i)
    {
        check = Scheduler::instance().getProcess(i);
        if(check->getType() != Process::Posix)
            continue;

        PosixProcess *posixCheck = static_cast<PosixProcess *>(check);
        ProcessGroup *pGroupCheck = posixCheck->getProcessGroup();
        if(pGroupCheck)
        {
            if(pGroupCheck->processGroupId == pgid)
            {
                // Join this group.
                pProcess->setProcessGroup(pGroupCheck);
                pProcess->setGroupMembership(PosixProcess::Member);
                return 0;
            }
        }
    }

    // No, the process group does not exist. Create it.
    ProcessGroup *pNewGroup = new ProcessGroup;
    pNewGroup->processGroupId = pProcess->getId();
    pNewGroup->Leader = pProcess;
    pNewGroup->Members.clear();

    // We're now a group leader - we got promoted!
    pProcess->setProcessGroup(pNewGroup);
    pProcess->setGroupMembership(PosixProcess::Leader);

    return 0;
}

int posix_getpgrp()
{
    SC_NOTICE("getpgrp");

    PosixProcess *pProcess = static_cast<PosixProcess *>(Processor::information().getCurrentThread()->getParent());
    ProcessGroup *pGroup = pProcess->getProcessGroup();

    int result = 0;
    if(pGroup)
        result = pProcess->getProcessGroup()->processGroupId;
    else
        result = pProcess->getId(); // Fallback if no ProcessGroup pointer yet

    SC_NOTICE(" -> " << result);
    return result;
}

int posix_syslog(const char *msg, int prio)
{
    if(!PosixSubsystem::checkAddress(reinterpret_cast<uintptr_t>(msg), PATH_MAX, PosixSubsystem::SafeRead))
    {
        SC_NOTICE("syslog -> invalid address");
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    uint64_t id = Processor::information().getCurrentThread()->getParent()->getId();
    if(id <= 1)
    {
        if(prio <= LOG_CRIT)
            FATAL("[" << Dec << id << Hex << "]\tsyslog: " << msg);
    }

    if(prio <= LOG_ERR)
        ERROR("[" << Dec << id << Hex << "]\tsyslog: " << msg);
    else if(prio == LOG_WARNING)
        WARNING("[" << Dec << id << Hex << "]\tsyslog: " << msg);
    else if(prio == LOG_NOTICE || prio == LOG_INFO)
        NOTICE("[" << Dec << id << Hex << "]\tsyslog: " << msg);
#if DEBUGGER
    else
        NOTICE("[" << Dec << id << Hex << "]\tsyslog: " << msg);
#endif
    return 0;
}

extern void system_reset();

int pedigree_reboot()
{
    WARNING("System shutting down...");
    for(int i = Scheduler::instance().getNumProcesses() - 1; i >= 0; i--)
    {
        // Grab the process and subsystem. Don't grab a POSIX subsystem object,
        // because we may be hitting native processes here.
        Process *proc = Scheduler::instance().getProcess(i);
        Subsystem *subsys = proc->getSubsystem();

        // DO NOT COMMIT SUICIDE. That's called a hang with undefined state, chilldren.
        if(proc == Processor::information().getCurrentThread()->getParent())
            continue;

        if(subsys)
        {
            // If there's a subsystem, kill it that way.
            /// \todo Proper KillReason
            subsys->kill(Subsystem::Unknown, proc->getThread(0));
        }
        else
        {
            // If no subsystem, outright kill the process without sending a signal
            Scheduler::instance().removeProcess(proc);

            /// \todo Process::kill() acts as if that process is already running.
            ///       It needs to allow other Processes to call it without causing
            ///       the calling thread to become a zombie.
            //proc->kill();
        }
    }

    // Wait for every other process to die or be in zombie state.

    while (true)
    {
        Processor::setInterrupts(false);
        if (Scheduler::instance().getNumProcesses() <= 1)
            break;
        bool allZombie = true;
        for (size_t i = 0; i < Scheduler::instance().getNumProcesses(); i++)
        {
            if (Scheduler::instance().getProcess(i) == Processor::information().getCurrentThread()->getParent())
                continue;
            if (Scheduler::instance().getProcess(i)->getThread(0)->getStatus() != Thread::Zombie)
                allZombie = false;
        }

        if (allZombie) break;
        Processor::setInterrupts(true);

        Scheduler::instance().yield();
    }

    // All dead, reap them all.
    while (Scheduler::instance().getNumProcesses() > 1)
    {
        if (Scheduler::instance().getProcess(0) == Processor::information().getCurrentThread()->getParent())
            continue;
        delete Scheduler::instance().getProcess(0);
    }

    // Reset the system
    system_reset();
    return 0;
}
