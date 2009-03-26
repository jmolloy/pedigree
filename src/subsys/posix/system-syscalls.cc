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
#include <vfs/VFS.h>
#include <panic.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/StackFrame.h>
#include <linker/DynamicLinker.h>
#include <utilities/String.h>
#include <utilities/Vector.h>
#include <machine/Machine.h>

// Defined in file-syscalls.cc
extern String prepend_cwd(const char *cwd);

//
// Syscalls pertaining to system operations.
//

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

int posix_sbrk(int delta)
{
  int ret = reinterpret_cast<int>(
    Processor::information().getVirtualAddressSpace().expandHeap (delta, VirtualAddressSpace::Write));

  if (ret == 0)
  {
    SYSCALL_ERROR(OutOfMemory);
    return -1;
  }
  else
    return ret;
}

int posix_fork(ProcessorState state)
{
  NOTICE("fork");
  
  Processor::setInterrupts(false);

  // Create a new process.
  Process *pProcess = new Process(Processor::information().getCurrentThread()->getParent());

  if (!pProcess)
  {
    SYSCALL_ERROR(OutOfMemory);
    return -1;
  }

  // Register with the dynamic linker.
  DynamicLinker::instance().registerProcess(pProcess);
  
  /// \todo All open descriptors need to be copied, not just stdin, stdout & stderr
  
  typedef Tree<size_t,void*> FdMap;
  FdMap parentFdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();
  FdMap childFdMap = pProcess->getFdMap();
  
  for(FdMap::Iterator it = parentFdMap.begin(); it != parentFdMap.end(); it++)
  {
    FileDescriptor* pFd = reinterpret_cast<FileDescriptor*> (it.value());
    if(!pFd)
      continue;
    size_t newFd = reinterpret_cast<size_t>(it.key());
    
    FileDescriptor *pFd2 = new FileDescriptor;

    // copyPipe will automatically handle the case where the file does not specify a pipe
    pFd2->file = PipeManager::instance().copyPipe(pFd->file);
    pFd2->offset = pFd->offset;
    pFd2->fd = pFd->fd;
    pFd2->fdflags = pFd->fdflags;
    pFd2->flflags = pFd->flflags;
    pProcess->getFdMap().insert(newFd, reinterpret_cast<void*> (pFd2));
    pProcess->nextFd(newFd + 1);
  }

  // Child returns 0.
  state.setSyscallReturnValue(0);

  // Create a new thread for the new process.
  new Thread(pProcess, state);

  // Parent returns child ID.
  return pProcess->getId();
}

int posix_execve(const char *name, const char **argv, const char **env, SyscallState &state)
{
  Processor::setInterrupts(false);
  NOTICE("execve");
  if (argv == 0 || env == 0)
  {
    SYSCALL_ERROR(ExecFormatError);
    return -1;
  }

  Process *pProcess = Processor::information().getCurrentThread()->getParent();

  // Ensure we only have one thread running (us).
  if (pProcess->getNumThreads() > 1)
  {
    SYSCALL_ERROR(ExecFormatError);
    return -1;
  }

  // Attempt to find the file, first!
  File* file = VFS::instance().find(prepend_cwd(name));

  if (!file->isValid())
  {
    // Error - not found.
    delete file;
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }
  if (file->isDirectory())
  {
    // Error - is directory.
    delete file;
    SYSCALL_ERROR(IsADirectory);
    return -1;
  }
  if (file->isSymlink())
  {
    /// \todo Not error - read in symlink and follow.
    delete file;
    SYSCALL_ERROR(Unimplemented);
    return -1;
  }

  // Attempt to load the file.
  uint8_t *buffer = new uint8_t[file->getSize()+1];

  if (buffer == 0)
  {
    SYSCALL_ERROR(OutOfMemory);
  }
  NOTICE("Reading file...");
  if (file->read(0, file->getSize(), reinterpret_cast<uintptr_t>(buffer)) != file->getSize())
  {
    delete [] buffer;
    delete file;
    SYSCALL_ERROR(TooBig);
    return -1;
  }
  NOTICE("File read.");
  Elf *elf = new Elf();
  if (!elf->create(buffer, file->getSize()))
  {
    // Error - bad file.
    delete [] buffer;
    delete file;
    SYSCALL_ERROR(ExecFormatError);
    return -1;
  }
  NOTICE("ELF created.");
  pProcess->description() = String(name);

  // Save the argv and env lists so they aren't destroyed when we overwrite the address space.
  Vector<String*> savedArgv, savedEnv;
  save_string_array(argv, savedArgv);
  save_string_array(env, savedEnv);

  // Get rid of all the crap from the last elf image.
  /// \todo Preserve anonymous mmaps etc.

  pProcess->getAddressSpace()->revertToKernelAddressSpace();

  uintptr_t loadBase;
  if (!elf->allocate(buffer, file->getSize(), loadBase))
  {
    // Error
    delete [] buffer;
    // Time to kill the task.
    ERROR("Could not allocate memory for ELF file.");

    delete file;
  
    return -1;
  }

  DynamicLinker::instance().unregisterProcess(pProcess);

  DynamicLinker::instance().registerElf(elf);

  List<char*> neededLibraries = elf->neededLibraries();
  for (List<char*>::Iterator it = neededLibraries.begin();
       it != neededLibraries.end();
       it++)
  {
    if (!DynamicLinker::instance().load(*it))
    {
      ERROR("Shared dependency '" << *it << "' not found.");

      delete file;
  
      return -1;
    }
  }
  NOTICE("ELF loaded.");
  elf->load(buffer, file->getSize(), loadBase, elf->getSymbolTable());

  DynamicLinker::instance().initialiseElf(elf);

  // Create a new stack.
  for (int j = 0; j < STACK_START-STACK_END; j += PhysicalMemoryManager::getPageSize())
  {
    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    bool b = Processor::information().getVirtualAddressSpace().map(phys,
                                                                   reinterpret_cast<void*> (j+STACK_END),
                                                                   VirtualAddressSpace::Write);
    if (!b)
      WARNING("map() failed in execve");
  }

  // Create room for the argv and env list.
  for (int j = 0; j < ARGV_ENV_LEN; j += PhysicalMemoryManager::getPageSize())
  {
    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    bool b = Processor::information().getVirtualAddressSpace().map(phys,
                                                                   reinterpret_cast<void*> (j+ARGV_ENV_LOC),
                                                                   VirtualAddressSpace::Write);
    if (!b)
      WARNING("map() failed in execve (2)");
  }

  // Load the saved argv and env into this address space, starting at "location".
  uintptr_t location = ARGV_ENV_LOC;
  argv = const_cast<const char**> (load_string_array(savedArgv, location, location));
  env  = const_cast<const char**> (load_string_array(savedEnv , location, location));

  ProcessorState pState = state;
  pState.setStackPointer(STACK_START-8);
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

  delete file;

  return 0;
}

int posix_waitpid(int pid, int *status, int options)
{
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
      if(pProcess == 0)
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
            delete pProcess;
            return pid;
          }
        }
      }
      if (!hadAnyChildren)
      {
        // Error - no children (ECHILD)
        return -1;
      }
    }

    if (options & 1) // WNOHANG set
      return 0;
    Scheduler::instance().yield(0);
  }

  return -1;
}

int posix_exit(int code)
{
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  pProcess->setExitStatus(code);

  DynamicLinker::instance().unregisterProcess(pProcess);

  pProcess->kill();

  // Should NEVER get here.
  for(;;);
}

int posix_getpid()
{
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  return pProcess->getId();
}

int posix_gettimeofday(timeval *tv, timezone *tz)
{
  Timer *pTimer = Machine::instance().getTimer();

  tv->tv_sec = pTimer->getYear() * (60*60*24*365) +
    pTimer->getMonth() * (60*60*24*30) +
    pTimer->getDayOfMonth() * (60*60*24) +
    pTimer->getHour() * (60*60) +
    pTimer->getMinute() * 60 +
    pTimer->getSecond();
  tv->tv_usec = 0;

  return 0;
}
