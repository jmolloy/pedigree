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
#include <vfs/VFS.h>
#include <panic.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/StackFrame.h>
#include <linker/DynamicLinker.h>
#include <utilities/String.h>
#include <utilities/Vector.h>
#include <machine/Machine.h>

#include <users/UserManager.h>

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
  
  typedef Tree<size_t,FileDescriptor*> FdMap;
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
    pProcess->getFdMap().insert(newFd, pFd2);
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
  File* file = VFS::instance().find(String(name), Processor::information().getCurrentThread()->getParent()->getCwd());

  if (!file->isValid())
  {
    // Error - not found.
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }
  if (file->isDirectory())
  {
    // Error - is directory.
    SYSCALL_ERROR(IsADirectory);
    return -1;
  }
  if (file->isSymlink())
  {
    /// \todo Not error - read in symlink and follow.
    SYSCALL_ERROR(Unimplemented);
    return -1;
  }

  // Attempt to load the file.
  uint8_t *buffer = new uint8_t[file->getSize()+1];
  NOTICE("getSize: " << file->getSize());
  if (buffer == 0)
  {
    SYSCALL_ERROR(OutOfMemory);
  }
  NOTICE("Reading file...");
  if (file->read(0, file->getSize(), reinterpret_cast<uintptr_t>(buffer)) != file->getSize())
  {
    delete [] buffer;
    SYSCALL_ERROR(TooBig);
    return -1;
  }
  NOTICE("File read.");
  Elf *elf = new Elf();
  if (!elf->create(buffer, file->getSize()))
  {
    // Error - bad file.
    delete [] buffer;
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

      return -1;
    }
  }
  NOTICE("ELF loaded.");
  elf->load(buffer, file->getSize(), loadBase, elf->getSymbolTable());

  // Close all FD_CLOEXEC descriptors. Done here because from this point we're committed to running -
  // there's no further return until the end of the function.
  typedef Tree<size_t,FileDescriptor*> FdMap;
  FdMap parentFdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();
  for(FdMap::Iterator it = parentFdMap.begin(); it != parentFdMap.end(); it++)
  {
    FileDescriptor* pFd = reinterpret_cast<FileDescriptor*> (it.value());
    if(pFd)
      if(pFd->fdflags & FD_CLOEXEC)
        posix_close(reinterpret_cast<int>(it.key()));
  }

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

  int sigret_func = ((1 & 0xFFFF) << 16) | (PEDIGREE_SIGRET & 0xFFFF);

  /// \todo Make this better.
  char tmpCode[] = {0xb8, sigret_func & 0xff, (sigret_func >> 8) & 0xff, (sigret_func >> 16) & 0xff, (sigret_func >> 24) & 0xff, 0xcd, 0xff};
  physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
  Processor::information().getVirtualAddressSpace().map(phys,
                                                        reinterpret_cast<void*> (0x50000000),
                                                        VirtualAddressSpace::Write);
  memcpy(reinterpret_cast<void*>(0x50000000), tmpCode, 7);

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
  return Processor::information().getCurrentThread()->getParent()->getUser()->getId();
}

int posix_getgid()
{
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

/* Signals stuff
(9:12:26 AM) JamesM: add a list of pending signals for each process. When a thread is scheduled, check the pending signal list for its process and run one and only one of them
(9:12:50 AM) pcmattman: ok cool
(9:12:52 AM) JamesM: do that by constructing an InterruptState that jumps to your special signal dispatcher in user mode
(9:13:17 AM) JamesM: and set the registers in the interrupt state such that you can reconstruct the original interrupt state when you return from that stub
(9:13:25 AM) JamesM: then let the scheduler context switch to that instead
(9:13:53 AM) JamesM: see Thread() for an example of using StackFrame and InterruptState to create a cross-platform version
(9:14:04 AM) JamesM: StackFrame should make it easy
(9:14:10 AM) pcmattman: awesome - thanks
(9:14:15 AM) JamesM: np
(9:14:37 AM) JamesM: the only thing I will say is - be aware that some architectures pass most things around in registers, not on the stack
(9:14:39 AM) JamesM: :-)
(9:14:56 AM) pcmattman: right
(9:15:31 AM) froggey: in addition to that, you have to save the current signal mask, mask and unmask some signals, and pass signal information + user context to the handler. you may also need to switch stacks and restart interrupted system calls
(9:15:31 AM) JamesM: ping me if you need me
(9:15:43 AM) froggey: plus other stuff
(9:15:55 AM) pcmattman: yeah
(9:16:00 AM) pcmattman: that's the complicated part
(9:16:04 AM) JamesM: interrupted system calls should be dealt with automatically by the scheduler
(9:16:19 AM) JamesM: InterruptState gets saved, all you have to do is jump to it
(9:16:47 AM) pcmattman: i know that read/write are meant to set errno to EINTR and return the number of bytes read until interrupted
(9:16:55 AM) froggey: then suddenly you're left with a half completed system call
(9:17:53 AM) TkTech: my networks likely to go down a few times
(9:18:01 AM) JamesM: have a setjmp() style thing at the start of each syscall
(9:18:09 AM) TkTech: re-organizing and bridging a new rouer
(9:18:20 AM) JamesM: with a check for interruption after it?
(9:20:10 AM) froggey: the way most (all?) implementations deal with it is to never switch to a signal handler while running a system call, but instead have some kind of interruptable sleep state which tells the sleeper that sleep was interrupted due to a signal
(9:20:24 AM) froggey: then the syscall can rewind changes and return EINTR from there
(9:20:59 AM) froggey: system calls that don't sleep aren't a problem, just let them run to the userspace return uninterrupted and then set up the syscall as normal there
(9:21:06 AM) JamesM: or posix can get fucked and we can ignore interrupted system calls
(9:21:28 AM) JamesM: nah I suppose we can't really...
(9:22:09 AM) froggey: if, for example, process A is blocked on a socket and you want to kill it, then this is where interruptable syscalls are important
(9:22:15 AM) JamesM: yeah
*/

/* SIGRET stub code


b8 ff 00 00 00
cd ff

*/

int posix_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
  NOTICE("sigaction");
  return -1;
}

uintptr_t posix_signal(int sig, void* func)
{
  NOTICE("signal");
  void* oldSignalHandler = Processor::information().getCurrentThread()->getParent()->getSignalHandler(static_cast<size_t>(sig));
  Processor::information().getCurrentThread()->getParent()->setSignalHandler(static_cast<size_t>(sig), func);
  return reinterpret_cast<uintptr_t>(oldSignalHandler);
}

int posix_raise(int sig)
{
  NOTICE("raise");

  // add the signal to the queue
  Processor::information().getCurrentThread()->getParent()->addPendingSignal(static_cast<size_t>(sig));

  // let another thread be scheduled; we won't come back until after the pending signal is handled
  Scheduler::instance().yield(0);

  // reset the saved state so reschedules work properly
  Processor::information().getCurrentThread()->setSavedInterruptState(0);

  return 0;
}

int pedigree_sigret()
{
  NOTICE("pedigree_sigret");

  // we do not want an interrupt until we're waiting for the signal handler to complete
  bool bInterrupts = Processor::getInterrupts();
  if(bInterrupts)
    Processor::setInterrupts(false);

  // grab the old state, verify it, and promptly switch to it
  InterruptState* oldState = Processor::information().getCurrentThread()->getSavedInterruptState();
  InterruptState* currState = Processor::information().getCurrentThread()->getInterruptState();
  Processor::information().getCurrentThread()->useSaved(true);

  // and now that we're done fiddling with the thread let us be interrupted
  if(bInterrupts)
    Processor::setInterrupts(true);

  // reschedule, when we get back to this thread it'll load the old state
  Scheduler::instance().yield(0);
  while(1)
    Processor::halt();

  return 0;
}

int posix_kill(int pid, int sig)
{
  Process* p = Scheduler::instance().getProcess(static_cast<size_t>(pid));
  p->addPendingSignal(static_cast<size_t>(sig));

  return 0;
}
