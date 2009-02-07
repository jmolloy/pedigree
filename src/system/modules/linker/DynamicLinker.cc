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

uintptr_t DynamicLinker::resolve(const char *str)
{
  return DynamicLinker::instance().resolveSymbol(str);
}

uintptr_t DynamicLinker::resolvePlt(SyscallState &state)
{
  return DynamicLinker::instance().resolvePltSymbol(state.getSyscallParameter(0), state.getSyscallParameter(1));
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

bool DynamicLinker::load(const char *name, Process *pProcess)
{
  m_pInitProcess = pProcess;
  if (!pProcess) pProcess = Processor::information().getCurrentThread()->getParent();

  SharedObject *pSo = loadInternal (name);

  if (pSo)
  {
    List<SharedObject*> *pList = m_ProcessObjects.lookup(pProcess);
    if (!pList)
    {
      pList = new List<SharedObject*>();
      m_ProcessObjects.insert(pProcess, pList);
    }
    pList->pushBack(pSo);

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
  File file = VFS::instance().find(String(fileName));

  if (m_pInitProcess)
    Processor::switchAddressSpace(oldAS);

  if (!file.isValid())
  {
    if (m_pInitProcess)
      Processor::switchAddressSpace(oldAS);

    ERROR("Unable to load object \"" << name << "\"");
    return 0;
  }

  uint8_t *buffer = new uint8_t[file.getSize()];
  file.read(0, file.getSize(), reinterpret_cast<uintptr_t>(buffer));

  if (m_pInitProcess)
    Processor::switchAddressSpace(oldAS);

  SharedObject *pSo = new SharedObject;
  pSo->pFile = new Elf();
  pSo->pFile->create(buffer, file.getSize());
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
  if (!pSo->pFile->allocate(buffer, file.getSize(), loadBase, pProcess))
  {
    ERROR("LINKER: nowhere to put shared object \"" << name << "\"");
    return 0;
  }

  if (!pSo->pFile->load(buffer, file.getSize(), loadBase, &resolve))
  {
    ERROR("LINKER: load() failed for object \"" << name << "\"");
  }

  pSo->pBuffer = buffer;
  pSo->nBuffer = file.getSize();
  initPlt(pSo->pFile, loadBase);

  pSo->addresses.insert(pProcess, reinterpret_cast<uintptr_t*>(loadBase));

  m_Objects.pushBack(pSo);

  return pSo;
}

DynamicLinker::SharedObject *DynamicLinker::mapObject(SharedObject *pSo)
{
  Process *pProcess = m_pInitProcess;
  if (!pProcess) pProcess = Processor::information().getCurrentThread()->getParent();

  // Load the dependencies. These should already be loaded - loadInternal will fall through to us (mapObject) again.
  for (int i = 0; i < pSo->nDependencies; i++)
  {
    mapObject(pSo->pDependencies[i]);
  }

  /// \todo Change this to using CoW, when we finally implement it :(
  uintptr_t loadBase = 0;
  if (!pSo->pFile->allocate(pSo->pBuffer, pSo->nBuffer, loadBase, pProcess))
  {
    ERROR("LINKER: nowhere to put shared object.");
    return 0;
  }

  if (!pSo->pFile->load(pSo->pBuffer, pSo->nBuffer, loadBase, &resolve))
  {
    ERROR("LINKER: load() failed for object.");
  }

  initPlt(pSo->pFile, loadBase);

  pSo->addresses.insert(pProcess, reinterpret_cast<uintptr_t*>(loadBase));

  return pSo;
}

uintptr_t DynamicLinker::resolveSymbol(const char *sym)
{
  Process *pProcess = m_pInitProcess;
  if (!pProcess) pProcess = Processor::information().getCurrentThread()->getParent();

  List<SharedObject*> *pList;

  pList = m_ProcessObjects.lookup(pProcess);

  // Look through the shared object list.
  for (List<SharedObject*>::Iterator it = pList->begin();
       it != pList->end();
       it++)
  {
    uintptr_t ptr = resolveInLibrary(sym, *it);
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

Elf32 *DynamicLinker::findElf(uintptr_t libraryId, SharedObject *pSo, uintptr_t &_loadBase)
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
    Elf32 *pElf = findElf(libraryId, pSo->pDependencies[i], _loadBase);
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

void DynamicLinker::registerElf(Elf32 *pElf)
{
  Process *pProcess = m_pInitProcess;
  if (!pProcess) pProcess = Processor::information().getCurrentThread()->getParent();

  m_ProcessElfs.remove(pProcess);
  m_ProcessElfs.insert(pProcess, pElf);

  initPlt(pElf, 0);
  m_pInitProcess = 0;
}

void DynamicLinker::registerProcess(Process *pProcess)
{
  Elf32 *pElf = m_ProcessElfs.lookup(Processor::information().getCurrentThread()->getParent());
  m_ProcessElfs.insert(pProcess, pElf);

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
