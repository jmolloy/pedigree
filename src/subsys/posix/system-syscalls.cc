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
#include <processor/types.h>
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/state.h>
#include <process/Process.h>
#include <process/Thread.h>
#include <process/Scheduler.h>
#include <Log.h>
#include <Elf32.h>
#include <vfs/File.h>
#include <vfs/VFS.h>
#include <panic.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/StackFrame.h>

//
// Syscalls pertaining to system operations.
//

int posix_sbrk(int delta)
{
  return reinterpret_cast<int>(
    Processor::information().getVirtualAddressSpace().expandHeap (delta, VirtualAddressSpace::Write));
}

int posix_fork(ProcessorState state)
{
  Processor::setInterrupts(false);

  Process *pProcess = new Process(Processor::information().getCurrentThread()->getParent());

  // Copy over stdin, stdout & stderr.
  for (int i = 0; i < 3; i++)
  {
    FileDescriptor *pFd = reinterpret_cast<FileDescriptor*> (Processor::information().getCurrentThread()->getParent()->getFdMap().lookup(i));
    FileDescriptor *pFd2 = new FileDescriptor;
    pFd2->file = pFd->file;
    pFd2->offset = pFd->offset;
    pProcess->getFdMap().insert(i, reinterpret_cast<void*> (pFd2));
    pProcess->nextFd();
  }

  state.setSyscallReturnValue(0); // Child returns 0.
  Thread *pThread = new Thread(pProcess, state);

  return pProcess->getId(); // Parent returns child ID.
}

int posix_execve(const char *name, const char **argv, const char **env, SyscallState &state)
{
  Processor::setInterrupts(false);
  NOTICE("Execve: "<<name);
  if (argv == 0/* || env == 0*/)
  {
    NOTICE("Argv = 0");
    return -1;
  }

  Process *pProcess = Processor::information().getCurrentThread()->getParent();

  // Ensure we only have one thread running (us).
  if (pProcess->getNumThreads() > 1)
  {
    FATAL("Execve with multiple threads!");
    Processor::breakpoint();
  }

  // Attempt to find the file, first!
  const char *cwd = Processor::information().getCurrentThread()->getParent()->getCwd();

  char *newName = new char[2048];
  newName[0] = '\0';

  // If the string starts with a '/', add on the current working filesystem.
  if (name[0] == '/')
  {
    int i = 0;
    while (cwd[i] != ':')
      newName[i] = cwd[i++];
    newName[i++] = ':';
    newName[i] = '\0';
    strcat(newName, name);
  }

  if (newName[0] == '\0')
    strcat(newName, name);

  // If the name doesn't contain a colon, add the cwd.
  int i = 0;
  bool contains = false;
  while (newName[i])
  {
    if (newName[i++] == ':')
    {
      contains = true;
      break;
    }
  }

  if (!contains)
  {
    newName[0] = '\0';
    strcat(newName, cwd);
    strcat(newName, name);
  }

  File file = VFS::instance().find(String(newName));

  delete [] newName;

  if (!file.isValid())
  {
    // Error - not found.
    NOTICE("Not found");
    return -1;
  }
  if (file.isDirectory())
  {
    // Error - is directory.
    return -1;
  }
  if (file.isSymlink())
  {
    /// \todo Not error - read in symlink and follow.
    return -1;
  }

  // Attempt to load the file.
  uint8_t *buffer = new uint8_t[file.getSize()+1];

  file.read(0, file.getSize(), reinterpret_cast<uintptr_t>(buffer));

  Elf32 elf;
  if (!elf.load(buffer, file.getSize()))
  {
    // Error - bad file.
    delete [] buffer;
    NOTICE("Bad file.");
    return -1;
  }

  pProcess->description() = String(name);

  char **new_argv = new char *[1024];
  char *loc = new char[4096*3];
  char *saved_loc1 = loc;

  i = 0;
  while (*argv)
  {
    new_argv[i++] = loc;
    strcpy(loc, *argv);
    loc += strlen(*argv);
    *loc++ = '\0'; // Make sure the string is null-terminated.
    argv++;
  }
  new_argv[i] = 0;

  char **saved_argv = new_argv;
  argv = const_cast<const char**>(new_argv);

  char **new_env = new char *[1024];
  loc = new char[4096*3];
  char *saved_loc2 = loc;

  i = 0;
  while (*env)
  {
    new_env[i++] = loc;
    strcpy(loc, *env);
    loc += strlen(*env);
    *loc++ = '\0'; // Make sure the string is null-terminated.
    env++;
  }
  new_env[i] = 0;

  char **saved_env = new_env;
  env = const_cast<const char**>(new_env);

  // Get rid of all the crap from the last elf image.
  /// \todo Preserve anonymous mmaps etc.

  pProcess->getAddressSpace()->revertToKernelAddressSpace();

  if (!elf.allocateSections())
  {
    // Error
    //delete [] buffer;
    // Time to kill the task.
    NOTICE("Kill task here");
    panic("Kill task here");
    return -1;
  }

  if (!elf.writeSections())
  {
    // Error.
    //delete [] buffer;
    NOTICE("Kill task here (2)");
    panic("Kill task here (2)");
    return -1;
  }

  // Create a new stack.
  for (int j = 0; j < 0x20000; j += 0x1000)
  {
    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    bool b = Processor::information().getVirtualAddressSpace().map(phys,
                                                                   reinterpret_cast<void*> (j+0x40000000),
                                                                   VirtualAddressSpace::Write);
    if (!b)
    WARNING("map() failed in execve");
  }

  // Create room for the argv and env list.
  for (int j = 0; j < 0x8000; j += 0x1000)
  {
    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    bool b = Processor::information().getVirtualAddressSpace().map(phys,
                                                                   reinterpret_cast<void*> (j+0x40100000),
                                                                   VirtualAddressSpace::Write);
    if (!b)
    WARNING("map() failed in execve");
  }

  new_argv = reinterpret_cast<char**>(0x40100000);
  loc = reinterpret_cast<char*>(0x40101000);

  i = 0;
  while (*argv)
  {
    new_argv[i++] = loc;
    strcpy(loc, *argv);
    loc += strlen(*argv);
    *loc++ = '\0'; // Make sure the string is null-terminated.
    argv++;
  }
  new_argv[i] = 0;

  new_env = reinterpret_cast<char**>(loc);
  loc += 1024;

  i = 0;
  while (*env)
  {
    new_env[i++] = loc;
    strcpy(loc, *env);
    loc += strlen(*env);
    *loc++ = '\0'; // Make sure the string is null-terminated.
    env++;
  }
  new_env[i] = 0;

  NOTICE("Deleting saved buffers");
  delete [] saved_argv;
  NOTICE("1");
  delete [] saved_env;
  NOTICE("2");
  delete [] saved_loc1;
  NOTICE("3");
  delete [] saved_loc2;
  NOTICE("Execve: all good!");
  ProcessorState pState = state;
  pState.setStackPointer(0x40020000-8);
  pState.setInstructionPointer(elf.getEntryPoint());

  StackFrame::construct(pState, 0, 2, new_argv, new_env);

  state.setStackPointer(pState.getStackPointer());
  state.setInstructionPointer(elf.getEntryPoint());

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
  NOTICE("waitpid(" << pid << ", " << (uintptr_t)status << ", " << options << ")");
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
      Process *pProcess;
      for (int i = 0; i < Scheduler::instance().getNumProcesses(); i++)
      {
        pProcess = Scheduler::instance().getProcess(i);

        if (pProcess->getId() == pid)
          break;
      }

      if (pProcess->getId() != pid)
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
      for (int i = 0; i < Scheduler::instance().getNumProcesses(); i++)
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
  pProcess->kill();

  // Should NEVER get here.
  for(;;);
}

int posix_getpid()
{
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  return pProcess->getId();
}

