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

#include "file-syscalls.h"
#include "system-syscalls.h"
#include "pipe-syscalls.h"
#include "syscallNumbers.h"
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
#include <vfs/File.h>
#include <vfs/Symlink.h>
#include <vfs/VFS.h>
#include <panic.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/StackFrame.h>
#include <linker/DynamicLinker.h>
#include <utilities/String.h>
#include <utilities/Vector.h>
#include <machine/Machine.h>

#include <Subsystem.h>
#include <PosixSubsystem.h>
#include <PosixProcess.h>

#include <users/UserManager.h>
//
// Syscalls pertaining to system operations.
//

#define GET_CWD() (Processor::information().getCurrentThread()->getParent()->getCwd())

/// Saves a char** array in the Vector of String*s given.
static void save_string_array(const char **array, Vector<String*> &rArray)
{
    while (*array)
    {
        String *pStr = new String(*array);
        rArray.pushBack(pStr);
        array++;
    }
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
    long ret = reinterpret_cast<long>(
                   Processor::information().getVirtualAddressSpace().expandHeap (delta, VirtualAddressSpace::Write));
    SC_NOTICE("sbrk(" << delta << ") -> " << ret);
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
        return -1;
    }
    pProcess->setSubsystem(pSubsystem);

    // Copy POSIX Process Group information if needed
    /// \todo Get a response on Stack Overflow and possibly put into the PosixProcess copy constructor
    if(pParentProcess->getType() == Process::Posix)
    {
        PosixProcess *p = static_cast<PosixProcess*>(pParentProcess);
        pProcess->setProcessGroup(p->getProcessGroup());

        // Do not adopt leadership status.
        if(p->getGroupMembership() == PosixProcess::Leader)
        {
            NOTICE("fork parent was a group leader.");
            pProcess->setGroupMembership(PosixProcess::Member);
        }
        else
        {
            NOTICE("fork parent had status " << static_cast<int>(p->getGroupMembership()) << "...");
            pProcess->setGroupMembership(p->getGroupMembership());
        }
    }

    // Register with the dynamic linker.
    pProcess->setLinker(new DynamicLinker(*pProcess->getLinker()));

    MemoryMappedFileManager::instance().clone(pProcess);

    // Copy the file descriptors from the parent
    pSubsystem->copyDescriptors(pParentSubsystem);

    // Child returns 0.
    state.setSyscallReturnValue(0);

    // Create a new thread for the new process.
    new Thread(pProcess, state);

    // Parent returns child ID.
    return pProcess->getId();
}

int posix_execve(const char *name, const char **argv, const char **env, SyscallState &state)
{
    SC_NOTICE("execve(\"" << name << "\")");

    String myArgv;
    int i = 0;
    while (argv[i])
    {
        myArgv += String(argv[i]);
        myArgv += " ";
        i++;
    }
    SC_NOTICE("  {" << myArgv << "}");

    Processor::setInterrupts(false);

    if (argv == 0 || env == 0)
    {
        SYSCALL_ERROR(ExecFormatError);
        return -1;
    }

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    pProcess->getSpaceAllocator().clear();
    pProcess->getSpaceAllocator().free(0x00100000, 0x80000000);
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Ensure we only have one thread running (us).
    if (pProcess->getNumThreads() > 1)
    {
        SYSCALL_ERROR(ExecFormatError);
        return -1;
    }

    // Attempt to find the file, first!
    File* file = VFS::instance().find(String(name), Processor::information().getCurrentThread()->getParent()->getCwd());
    if (!file)
    {
        // Error - not found.
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }

    while (file->isSymlink())
        file = Symlink::fromFile(file)->followLink();

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
    String theShebang, *oldPath = 0;
    static char tmpBuff[128 + 1];
    file->read(0, 128, reinterpret_cast<uintptr_t>(tmpBuff));
    if (!strncmp(tmpBuff, "#!", 2))
    {
        // We have a shebang, so grab the command
        /// \todo Handle shebang lines that contain arguments (eg, "#!/bin/python -i")
        NormalStaticString execLine;
        execLine.clear();

        size_t i = 2;
        while (i < file->getSize())
        {
            file->read(i, 128, reinterpret_cast<uintptr_t>(tmpBuff));
            tmpBuff[128] = 0;
            execLine += tmpBuff;

            if (execLine.contains("\n"))
            {
                execLine = execLine.left(execLine.first('\n'));
                break;
            }

            i += 128;
        }

        execLine += "\0";

        oldPath = new String(name);
        savedArgv.pushBack(oldPath);

        theShebang = String(static_cast<const char*>(execLine));
        name = static_cast<const char*>(theShebang);

        // And reload the file, now that we're loading a new application
        file = VFS::instance().find(theShebang, Processor::information().getCurrentThread()->getParent()->getCwd());
    }

    pProcess->description() = String(name);

    // Save the argv and env lists so they aren't destroyed when we overwrite the address space.
    save_string_array(argv, savedArgv);
    save_string_array(env, savedEnv);

    // Get rid of all the crap from the last elf image.
    /// \todo Preserve anonymous mmaps etc.

    MemoryMappedFileManager::instance().unmapAll();

    pProcess->getAddressSpace()->revertToKernelAddressSpace();

    DynamicLinker *pOldLinker = pProcess->getLinker();
    DynamicLinker *pLinker = new DynamicLinker();

    // Set the new linker now before we loadProgram, else we could trap and
    // have a linker mismatch.
    pProcess->setLinker(pLinker);

    if (!pLinker->loadProgram(file))
    {
        pProcess->setLinker(pOldLinker);
        delete pLinker;
        SYSCALL_ERROR(ExecFormatError);
        return -1;
    }
    delete pOldLinker;

    // Close all FD_CLOEXEC descriptors.
    pSubsystem->freeMultipleFds(true);

    // Create a new stack.
    uintptr_t newStack = reinterpret_cast<uintptr_t>(Processor::information().getVirtualAddressSpace().allocateStack());

    // Create room for the argv and env list.
    for (int j = 0; j < ARGV_ENV_LEN; j += PhysicalMemoryManager::getPageSize())
    {
        void *pVirt = reinterpret_cast<void*> (j+ARGV_ENV_LOC);
        if (!Processor::information().getVirtualAddressSpace().isMapped(pVirt))
        {
            physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
            bool b = Processor::information().getVirtualAddressSpace().map(phys,
                                                                           pVirt,
                                                                           VirtualAddressSpace::Write);
            if (!b)
                WARNING("map() failed in execve (2)");
        }
    }

    // Load the saved argv and env into this address space, starting at "location".
    uintptr_t location = ARGV_ENV_LOC;
    argv = const_cast<const char**> (load_string_array(savedArgv, location, location));
    env  = const_cast<const char**> (load_string_array(savedEnv , location, location));

    Elf *elf = pProcess->getLinker()->getProgramElf();

    ProcessorState pState = state;
    //pState.setStackPointer(STACK_START-8);
    pState.setStackPointer(newStack-8);
    pState.setInstructionPointer(elf->getEntryPoint());

    StackFrame::construct(pState, 0, 2, argv, env);

    state.setStackPointer(pState.getStackPointer());
    state.setInstructionPointer(elf->getEntryPoint());

    /// \todo Genericize this somehow - "pState.setScratchRegisters(state)"?
#ifdef PPC_COMMON
    state.m_R6 = pState.m_R6;
    state.m_R7 = pState.m_R7;
    state.m_R8 = pState.m_R8;
#endif

    return 0;
}

int posix_waitpid(int pid, int *status, int options)
{
    if (options & 1)
    {
        SC_NOTICE("waitpid(pid=" << Dec << pid << Hex << ", WNOHANG)");
    }
    else
    {
        SC_NOTICE("waitpid(pid=" << Dec << pid << Hex << ")");
    }

    // Don't care about process groups at the moment.
    if (pid < -1)
    {
        pid *= -1;
    }

    while (1)
    {
        // Is the pid an absolute pid reference?
        if (pid > 0)
        {
            // Does this process exist?
            Process *pProcess = 0;
            for (size_t i = 0; i < Scheduler::instance().getNumProcesses(); i++)
            {
                pProcess = Scheduler::instance().getProcess(i);

                if (static_cast<int>(pProcess->getId()) == pid)
                    break;
            }
            if (pProcess == 0)
                return -1;

            if (static_cast<int>(pProcess->getId()) != pid)
            {
                // ECHILD - process n'existe pas.
                return -1;
            }

            // Is it actually our child?
            if (pProcess->getParent() != Processor::information().getCurrentThread()->getParent())
            {
                // ECHILD - not our child!
                return -1;
            }

            // Is it zombie?
            if (pProcess->getThread(0)->getStatus() == Thread::Zombie)
            {
                // Ph33r the Reaper...
                if (status)
                    *status = pProcess->getExitStatus();

                // Delete the process; it's been reaped good and proper.
                NOTICE("Pid " << pid << " reaped");
                delete pProcess;
                return pid;
            }
        }
        else
        {
            // Get any pid.
            Process *thisP = Processor::information().getCurrentThread()->getParent();
            bool hadAnyChildren = false;
            for (size_t i = 0; i < Scheduler::instance().getNumProcesses(); i++)
            {
                Process *pProcess = Scheduler::instance().getProcess(i);

                if (pProcess->getParent() == thisP)
                {
                    hadAnyChildren = true;
                    if (pProcess->getThread(0)->getStatus() == Thread::Zombie)
                    {
                        // Kill 'em all!
                        if (status)
                            *status = pProcess->getExitStatus();
                        pid = pProcess->getId();
                        NOTICE("Pid " << pid << " reaped");
                        delete pProcess;
                        return pid;
                    }
                }
            }
            if (!hadAnyChildren)
            {
                // Error - no children (ECHILD)
                NOTICE("no children");
                SYSCALL_ERROR(NoChildren);
                return -1;
            }
        }

        if (options & 1) // WNOHANG set
            return 0;

        // Sleep...
        Processor::information().getCurrentThread()->getParent()->m_DeadThreads.acquire();
        SC_NOTICE("deadt");
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
    /// \note asm volatile
    for (;;) asm volatile("xor %eax, %eax");
}

int posix_getpid()
{
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    return pProcess->getId();
}

int posix_gettimeofday(timeval *tv, timezone *tz)
{
    Timer *pTimer = Machine::instance().getTimer();

    tv->tv_sec = pTimer->getUnixTimestamp();
    tv->tv_usec = 0;

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
    str = store_str_to(str, strend, pUser->getShell());

    return 0;
}

int posix_getpwnam(passwd *pw, const char *name, char *str)
{
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
    str = store_str_to(str, strend, pUser->getShell());

    return 0;
}

int posix_getuid()
{
    SC_NOTICE("getuid() -> " << Dec << Processor::information().getCurrentThread()->getParent()->getUser()->getId());
    return Processor::information().getCurrentThread()->getParent()->getUser()->getId();
}

int posix_getgid()
{
    SC_NOTICE("getgid() -> " << Dec << Processor::information().getCurrentThread()->getParent()->getGroup()->getId());
    return Processor::information().getCurrentThread()->getParent()->getGroup()->getId();
}

int pedigree_login(int uid, const char *password)
{
    // Grab the given user.
    User *pUser = UserManager::instance().getUser(uid);
    if (!pUser) return -1;

    if (pUser->login(String(password)))
        return 0;
    else
        return -1;
}

/// \note Currently all functionality can be provided without any extra storage in the handle.
struct dlHandle
{
    int mode;
};

uintptr_t posix_dlopen(const char* file, int mode, void* p)
{
    SC_NOTICE("dlopen(" << file << ")");

    File *pFile = VFS::instance().find(String(file), GET_CWD());

    if (!pFile)
    {
        SYSCALL_ERROR(DoesNotExist);
        return 0;
    }

    while (pFile->isSymlink())
        pFile = Symlink::fromFile(pFile)->followLink();

    if (pFile->isDirectory())
    {
        // Error - is directory.
        SYSCALL_ERROR(IsADirectory);
        return 0;
    }

    Process *pProcess = Processor::information().getCurrentThread()->getParent();

    if (!pProcess->getLinker()->loadObject(pFile))
    {
        ERROR("dlopen: couldn't load " << String(file) << ".");
        return 0;
    }

    dlHandle* handle = reinterpret_cast<dlHandle*>(p);

    return reinterpret_cast<uintptr_t>(handle);
}

// m_ProcessObjects should be better to use!

uintptr_t posix_dlsym(void* handle, const char* name)
{
    SC_NOTICE("dlsym(" << name << ")");

    if (!handle)
        return 0;

    Process *pProcess = Processor::information().getCurrentThread()->getParent();

    return pProcess->getLinker()->resolve(String(name));
}

int posix_dlclose(void* handle)
{
    SC_NOTICE("dlclose");

    return 0;
}

int posix_setsid()
{
    NOTICE("setsid");

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
            NOTICE("setsid() called while the leader of another group");
            SYSCALL_ERROR(PermissionDenied);
            return -1;
        }
        else
        {
            NOTICE("setsid() called while a member of another group");
        }
    }

    // Delete the old group, if any
    ProcessGroup *pGroup = pProcess->getProcessGroup();
    if(pGroup)
    {
        pProcess->setProcessGroup(0);

        /// \todo Remove us from the list
        if(pGroup->Members.count() <= 1) // Us or nothing
            delete pGroup;
    }

    // Create the actual group itself
    ProcessGroup *pNewGroup = new ProcessGroup;
    pNewGroup->processGroupId = pProcess->getId();
    pNewGroup->Leader = pProcess;
    pNewGroup->Members.clear();

    // We're now a group leader - we got promoted!
    pProcess->setProcessGroup(pNewGroup);
    pProcess->setGroupMembership(PosixProcess::Leader);

    NOTICE("Now part of a group [id=" << pNewGroup->processGroupId << "]!");

    // Success!
    return pNewGroup->processGroupId;
}

int posix_setpgid(int pid, int pgid)
{
    NOTICE("STUBBED setpgid");
    return 0;
}

int posix_getpgrp()
{
    NOTICE("getpgrp");

    PosixProcess *pProcess = static_cast<PosixProcess *>(Processor::information().getCurrentThread()->getParent());
    ProcessGroup *pGroup = pProcess->getProcessGroup();
    if(pGroup)
        return pProcess->getProcessGroup()->processGroupId;
    else
        return pProcess->getId(); // Fallback if no ProcessGroup pointer yet
}
