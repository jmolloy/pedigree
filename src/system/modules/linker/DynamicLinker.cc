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

#include "DynamicLinker.h"
#include <Log.h>
#include <vfs/VFS.h>
#include <utilities/StaticString.h>
#include <Module.h>
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/KernelCoreSyscallManager.h>
#include <process/Scheduler.h>
#include <panic.h>

DynamicLinker DynamicLinker::m_Instance;

uintptr_t DynamicLinker::resolve(const char *str, bool useElf)
{
  return DynamicLinker::instance().resolveSymbol(str, useElf);
}

uintptr_t DynamicLinker::resolveNoElf(const char *str, bool useElf)
{
  return DynamicLinker::instance().resolveSymbol(str, false);
}

uintptr_t DynamicLinker::resolvePlt(SyscallState &state)
{
  uintptr_t ret= DynamicLinker::instance().resolvePltSymbol(state.getSyscallParameter(0), state.getSyscallParameter(1));
  return ret;
}

DynamicLinker::DynamicLinker() :
  m_ProcessObjects(),
  m_Objects(),
  m_ProcessElfs(),
  m_pInitProcess(0)
{
  KernelCoreSyscallManager::instance().registerSyscall(KernelCoreSyscallManager::link, &resolvePlt);
}

DynamicLinker::~DynamicLinker()
{
}

void DynamicLinker::setInitProcess(Process *pProcess)
{
  m_pInitProcess = pProcess;
}

bool DynamicLinker::load(const char *name, Process *pProcess)
{
  if (!pProcess) pProcess = Processor::information().getCurrentThread()->getParent();

  SharedObject *pSo = loadInternal (name);

  if (pSo)
  {
    return true;
  }

  return false;
}

DynamicLinker::SharedObject *DynamicLinker::loadInternal(const char *name)
{
  // Search the currently loaded objects first.
  for (List<SharedObject*>::Iterator it = m_Objects.begin();
       it != m_Objects.end();
       it++)
  {
    SharedObject *pSo = *it;
    if (!strcmp(name, pSo->name))
    {
      return mapObject(pSo);
    }
  }

  // Have to load the object for the first time.
  return loadObject(name);
}

DynamicLinker::SharedObject *DynamicLinker::loadObject(const char *name)
{
  Process *pProcess = m_pInitProcess;
  if (!pProcess) pProcess = Processor::information().getCurrentThread()->getParent();

  // Create a fully-qualified library name.
  LargeStaticString fileName;
  fileName += "root:/libraries/";
  fileName += name;

  // Here we have a problem. If the init process requires shared libs, we'll be called from init.o.
  // HOWEVER, init.o has to switch address spaces to map things correctly, which normally is fine because interrupts
  // are disabled. When we find and read a file, multiple threads are used, which reenables interrupts. When the scheduler
  // switches back to us, it switches back to init.o's "real" address space, not the one it switched to!
  // So, we have to save the current address space here, and switch back to it at any time it could have been changed.
  VirtualAddressSpace &oldAS = Processor::information().getVirtualAddressSpace();

  // Attempt to open the file.
  File *file = VFS::instance().find(String(fileName));

  if (m_pInitProcess)
  {
    Processor::setInterrupts(false);
    Processor::switchAddressSpace(oldAS);
  }

  if (!file->isValid())
  {
    if (m_pInitProcess)
    {
      Processor::setInterrupts(false);
      Processor::switchAddressSpace(oldAS);
    }

    ERROR("Unable to load object \"" << name << "\"");
    return 0;
  }

  uint8_t *buffer = new uint8_t[file->getSize()];
  file->read(0, file->getSize(), reinterpret_cast<uintptr_t>(buffer));

  if (m_pInitProcess)
  {
    Processor::setInterrupts(false);
    Processor::switchAddressSpace(oldAS);
  }

  SymbolTable *pSymtab = m_ProcessElfs.lookup(pProcess)->getSymbolTable();

  SharedObject *pSo = new SharedObject;
  pSo->pFile = new Elf();
  pSo->pFile->create(buffer, file->getSize());
  pSo->refCount = 1;
  pSo->name = String(name);

  // Count the number of dependencies.
  List<char*> &dependencies = pSo->pFile->neededLibraries();
  int nDeps = dependencies.count();

  // Allocate space to store the dependencies.
  pSo->pDependencies = new SharedObject *[nDeps];
  pSo->nDependencies = nDeps;

  // Load the dependencies.
  /// \todo Check for cyclic dependencies.
  int i = 0;
  for (List<char*>::Iterator it = dependencies.begin();
       it != dependencies.end();
       it++,i++)
  {
    pSo->pDependencies[i] = loadInternal(*it);
  }

  uintptr_t loadBase = 0;
  if (!pSo->pFile->allocate(buffer, file->getSize(), loadBase, pSymtab, pProcess))
  {
    ERROR("LINKER: nowhere to put shared object \"" << name << "\"");
    return 0;
  }

  List<SharedObject*> *pList = m_ProcessObjects.lookup(pProcess);
  if (!pList)
  {
    pList = new List<SharedObject*>();
    m_ProcessObjects.insert(pProcess, pList);
  }
  pList->pushBack(pSo);

  pSo->addresses.insert(pProcess, reinterpret_cast<uintptr_t*>(loadBase));

  m_Objects.pushBack(pSo);

  if (!pSo->pFile->load(buffer, file->getSize(), loadBase, pSymtab))
  {
    ERROR("LINKER: load() failed for object \"" << name << "\"");
  }

  pSo->pBuffer = buffer;
  pSo->nBuffer = file->getSize();
  initPlt(pSo->pFile, loadBase);
  return pSo;
}

DynamicLinker::SharedObject *DynamicLinker::mapObject(SharedObject *pSo)
{
  Process *pProcess = Processor::information().getCurrentThread()->getParent();

  // Load the dependencies. These should already be loaded - loadInternal will fall through to us (mapObject) again.
  for (int i = 0; i < pSo->nDependencies; i++)
  {
    mapObject(pSo->pDependencies[i]);
  }

  SymbolTable *pSymtab = m_ProcessElfs.lookup(pProcess)->getSymbolTable();

  /// \todo Change this to using CoW, when we finally implement it :(
  uintptr_t loadBase = 0;
  if (!pSo->pFile->allocate(pSo->pBuffer, pSo->nBuffer, loadBase, pSymtab, pProcess))
  {
    ERROR("LINKER: nowhere to put shared object.");
    return 0;
  }

  List<SharedObject*> *pList = m_ProcessObjects.lookup(pProcess);
  if (!pList)
  {
    pList = new List<SharedObject*>();
    m_ProcessObjects.insert(pProcess, pList);
  }
  pList->pushBack(pSo);

  pSo->addresses.insert(pProcess, reinterpret_cast<uintptr_t*>(loadBase));

  if (!pSo->pFile->load(pSo->pBuffer, pSo->nBuffer, loadBase, pSymtab))
  {
    ERROR("LINKER: load() failed for object.");
  }

  initPlt(pSo->pFile, loadBase);

  return pSo;
}

uintptr_t DynamicLinker::resolveSymbol(const char *sym, bool useElf)
{
  Process *pProcess = m_pInitProcess;
  if (!pProcess) pProcess = Processor::information().getCurrentThread()->getParent();

  Elf *pElf = m_ProcessElfs.lookup(pProcess);
  if (!pElf)
  {
    ERROR("Resolve called on unregistered process");
    return 0;
  }

  if (useElf)
  {
    // LoadBase for the program itself is of course zero.
    uintptr_t addr = pElf->lookupDynamicSymbolAddress(sym,0);
    if (addr) {WARNING("Addr for " << sym);return addr;}
  }

  List<SharedObject*> *pList;

  pList = m_ProcessObjects.lookup(pProcess);
  if (!pList)
  {
    ERROR("Resolve called on nonregistered process");
    return 0;
  }
NOTICE("looking for " << sym);
  // Look through the shared object list.
  for (List<SharedObject*>::Iterator it = pList->begin();
       it != pList->end();
       it++)
  {
    uintptr_t ptr = resolveInLibrary(sym, *it);
    NOTICE("ptr: " <<Hex << ptr);
    if (ptr != 0) return ptr;
  }
}

uintptr_t DynamicLinker::resolveInLibrary(const char *sym, SharedObject *obj)
{
  Process *pProcess = m_pInitProcess;
  if (!pProcess) pProcess = Processor::information().getCurrentThread()->getParent();

  // Grab the load address of this object in this process.
  uintptr_t loadBase = reinterpret_cast<uintptr_t>
                         (obj->addresses.lookup(pProcess));
  // Sanity check
  if (loadBase == 0)
  {
    // Something went horrendously wrong!
    FATAL("DynamicLinker: Loadbase is 0!");
    panic("DynamicLinker: Loadbase is 0!");
  }

  uintptr_t addr = obj->pFile->lookupDynamicSymbolAddress(sym, loadBase);

  if (addr)
    return addr;

  // Not found - try the dependencies.
  for (int i = 0; i < obj->nDependencies; i++)
  {
    addr = resolveInLibrary(sym, obj->pDependencies[i]);
    if (addr)
      return addr;
  }

  // Failed.
  return 0;
}

Elf *DynamicLinker::findElf(uintptr_t libraryId, SharedObject *pSo, uintptr_t &_loadBase)
{
  // Grab the load address of this object in this process.
  uintptr_t loadBase = reinterpret_cast<uintptr_t>
                         (pSo->addresses.lookup(Processor::information().getCurrentThread()->getParent()));

  // Sanity check
  if (loadBase == 0)
  {
    // Something went horrendously wrong!
    FATAL("DynamicLinker: Loadbase is 0!");
    panic("DynamicLinker: Loadbase is 0!");
  }

  if (loadBase == libraryId)
  {
    _loadBase = loadBase;
    return pSo->pFile;
  }

  // Not found - try the dependencies.
  for (int i = 0; i < pSo->nDependencies; i++)
  {
    Elf *pElf = findElf(libraryId, pSo->pDependencies[i], _loadBase);
    if (pElf)
      return pElf;
  }

  // Failed.
  return 0;
}

void DynamicLinker::unregisterProcess(Process *pProcess)
{
  m_ProcessElfs.remove(pProcess);
  m_ProcessObjects.remove(pProcess);
  for (List<SharedObject*>::Iterator it = m_Objects.begin();
       it != m_Objects.end();
       it++)
  {
    SharedObject *pSo = *it;
    uintptr_t *ptr;
    if ((ptr=pSo->addresses.lookup(pProcess)))
    {
      pSo->refCount--; // Object no longer required.

      if (pSo->refCount == 0)
      {
        // Time to die, Mr. Bond...
        WARNING("Implement library unloading.");
      }

      pSo->addresses.remove(pProcess);
    }
  }
}

void DynamicLinker::registerElf(Elf *pElf)
{
  Process *pProcess = m_pInitProcess;
  if (!pProcess) pProcess = Processor::information().getCurrentThread()->getParent();

  m_ProcessElfs.remove(pProcess);
  m_ProcessElfs.insert(pProcess, pElf);
}

void DynamicLinker::initialiseElf(Elf *pElf)
{
  initPlt(pElf, 0);
}

void DynamicLinker::registerProcess(Process *pProcess)
{
  Elf *pElf = m_ProcessElfs.lookup(Processor::information().getCurrentThread()->getParent());
  m_ProcessElfs.insert(pProcess, pElf);
  ERROR("Register process: " << (uintptr_t)pElf << ", process: " << (uintptr_t)pProcess);
  for (List<SharedObject*>::Iterator it = m_Objects.begin();
       it != m_Objects.end();
       it++)
  {
    SharedObject *pSo = *it;
    uintptr_t *ptr;
    if ((ptr=pSo->addresses.lookup(Processor::information().getCurrentThread()->getParent())))
    {
      pSo->refCount++; // Another process requires this object.
      pSo->addresses.insert(pProcess, ptr);
    }
  }

  m_ProcessObjects.insert ( pProcess, m_ProcessObjects.lookup(Processor::information().getCurrentThread()->getParent()) );
}

void init()
{
}

void destroy()
{
}

MODULE_NAME("linker");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
MODULE_DEPENDS(0);
