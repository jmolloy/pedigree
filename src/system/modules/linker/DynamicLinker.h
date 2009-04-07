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

#ifndef DYNAMIC_LINKER_H
#define DYNAMIC_LINKER_H

#include <processor/types.h>
#include <linker/Elf.h>
#include <process/Process.h>
#include <utilities/Tree.h>
#include <utilities/List.h>
#include <processor/state.h>

/** The dynamic linker tracks instances of shared objects. */
class DynamicLinker
{
public:
  /** Retrieves the singleton instance. */
  static DynamicLinker &instance() {return m_Instance;}

  void setInitProcess(Process *pProcess);

  /** Loads a shared library into the current process' address space.

      If pProcess is non-zero, the linker assumes it is operating in pProcess'
      address space, not the current process'. This is the case in init, where
      the first process gets launched.

      The linker will assume this until registerElf is called. */
  bool load(const char *name, Process *pProcess=0);

  /** Registers the given Elf with the current Process, or load()'s pProcess if that was
      non-zero. */
  void registerElf(Elf *pElf);
  
  /** Initialises the ELF. */
  void initialiseElf(Elf *pElf);

  /** Registers the Elf belonging to the current Process to pProcess too. Used during fork(). */
  void registerProcess(Process *pProcess);

  /** Unloads all objects from this Process' address space. */
  void unregisterProcess(Process *pProcess);

  /** Dynamic resolver function, to be given to ElfXX::relocateDynamic. */
  uintptr_t resolve(const char *sym);
  //static uintptr_t resolveNoElf(const char *sym, bool useElf);

  /** Callback given to KernelCoreSyscallManager to resolve PLT relocations lazily. */
  static uintptr_t resolvePlt(SyscallState &state);

private:
  /** Constructor is private - singleton class. */
  DynamicLinker();
  /** Destructor is private - singleton class. */
  ~DynamicLinker();
  /** Copy constructor is private. */
  DynamicLinker(DynamicLinker &);
  /** As is operator= */
  DynamicLinker &operator=(const DynamicLinker&);

  /** A shared object/library. */
  class SharedObject
  {
  public:
    SharedObject() : pFile(0), name(), refCount(1), pDependencies(0), nDependencies(0), addresses(),
      pBuffer(0), nBuffer(0) {}
    Elf         *pFile;
    String         name;
    int            refCount;
    SharedObject **pDependencies;
    int            nDependencies;
    Tree<Process*,uintptr_t*> addresses; // For each process, the address this object has been relocated to.
    uint8_t       *pBuffer; // The Elf file itself.
    size_t         nBuffer; // The size of pBuffer.
  private:
    /** Copy constructor - it's private. It can't be used. */
    SharedObject (SharedObject &);
    SharedObject &operator=(const DynamicLinker&);
  };

  SharedObject *loadInternal (const char *name);
  SharedObject *loadObject (const char *name);
  SharedObject *mapObject (SharedObject *pSo);

  uintptr_t resolveInLibrary(const char *sym, SharedObject *obj);

  /** Resolves a reference. */
  uintptr_t resolveSymbol(const char *symbol, bool useElf);

  uintptr_t resolvePltSymbol(uintptr_t libraryId, uintptr_t symIdx);
  Elf *findElf(uintptr_t libraryId, SharedObject *pSo, uintptr_t &_loadBase);

  void initPlt(Elf *pElf, uintptr_t value);

  Tree<Process*,List<SharedObject*>*> m_ProcessObjects;
  List<SharedObject*> m_Objects;
  Tree<Process*,Elf*> m_ProcessElfs;

  /** Non-zero if load() was called with pProcess != 0, in which case it holds the value of pProcess. */
  Process *m_pInitProcess;

  static DynamicLinker m_Instance;
};

#endif
