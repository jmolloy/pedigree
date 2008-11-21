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

extern "C" void resolveSymbol(void);

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
    List<SharedObject*> *pList = new List<SharedObject*>();
    pList->pushBack(pSo);
    m_ProcessObjects.insert(pProcess, pList);

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
      // Map into this address space.
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
  pSo->pFile = new Elf32();
  pSo->pFile->load(buffer, file.getSize());
  pSo->refCount = 1;
  pSo->name = String(name);

  // Count the number of dependencies.
  uintptr_t iter = 0;
  int nDeps = 0;
  while (pSo->pFile->neededLibrary(iter))
    nDeps++;

  // Allocate space to store the dependencies.
  pSo->pDependencies = new SharedObject *[nDeps];
  pSo->nDependencies = nDeps;

  // Load the dependencies.
  /// \todo Check for cyclic dependencies.
  iter = 0;
  for (int i = 0; i < nDeps; i++)
  {
    const char *n = pSo->pFile->neededLibrary(iter);
    pSo->pDependencies[i] = loadInternal(n);
  }

  uintptr_t loadBase = 0;

  // Start at 0x20000000, looking for the next free page.
  /// \todo Change this to use the size of the elf!
  for (uintptr_t i = 0x20000000; i < 0x40000000; i += 0x1000) /// \todo Page size here.
  {
    bool failed = false;
    for (uintptr_t j = i; j < i+0x40000; j += 0x1000) /// \todo Page size here.
      if (oldAS.isMapped(reinterpret_cast<void*>(j)))
      {
        failed = true;
        break;
      }

    if (failed) continue;

    loadBase = i;
    break;
  }

  if (loadBase == 0)
  {
    ERROR("DynamicLinker: nowhere to put shared object \"" << name << "\"");
    return 0;
  }

  pSo->pFile->setLoadBase(loadBase);
  pSo->pFile->allocateSegments();
  pSo->pFile->writeSegments();
  pSo->pFile->relocateDynamic(&resolve);
  pSo->pBuffer = buffer;
  setGot(pSo->pFile, loadBase);

  pSo->addresses.insert(pProcess, reinterpret_cast<uintptr_t*>(loadBase));

  m_Objects.pushBack(pSo);

  return pSo;
}

DynamicLinker::SharedObject *DynamicLinker::mapObject(SharedObject *pSo)
{
  Process *pProcess = m_pInitProcess;
  if (!pProcess) pProcess = Processor::information().getCurrentThread()->getParent();

  // Here we have a problem. If the init process requires shared libs, we'll be called from init.o.
  // HOWEVER, init.o has to switch address spaces to map things correctly, which normally is fine because interrupts
  // are disabled. When we find and read a file, multiple threads are used, which reenables interrupts. When the scheduler
  // switches back to us, it switches back to init.o's "real" address space, not the one it switched to!
  // So, we have to save the current address space here, and switch back to it at any time it could have been changed.
  VirtualAddressSpace &oldAS = Processor::information().getVirtualAddressSpace();
  
  uintptr_t loadBase = 0;

  // Start at 0x20000000, looking for the next free page.
  /// \todo Change this to use the size of the elf!
  for (uintptr_t i = 0x20000000; i < 0x40000000; i += 0x1000) /// \todo Page size here.
  {
    bool failed = false;
    for (uintptr_t j = i; j < i+0x40000; j += 0x1000) /// \todo Page size here.
      if (oldAS.isMapped(reinterpret_cast<void*>(j)))
      {
        failed = true;
        break;
      }

    if (failed) continue;

    loadBase = i;
    break;
  }

  if (loadBase == 0)
  {
    ERROR("DynamicLinker: nowhere to put shared object");
    return 0;
  }

  pSo->pFile->setLoadBase(loadBase);
  pSo->pFile->allocateSegments();
  pSo->pFile->writeSegments();
  pSo->pFile->relocateDynamic(&resolve);
  setGot(pSo->pFile, loadBase);

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

  /// \todo Terrible terrible reentrancy related race conditions here!
  obj->pFile->setLoadBase(loadBase);
  uintptr_t addr = obj->pFile->lookupDynamicSymbolAddress(sym);

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

void DynamicLinker::setGot(Elf32 *pElf, uintptr_t value)
{
  uint32_t *got = reinterpret_cast<uint32_t*> (pElf->getGlobalOffsetTable());
  if (!got)
  {
    ERROR("DynamicLinker: Global offset table not found!");
    return;
  }

  got++;                     // Go to GOT+4
  *got = value&0xFFFFFFFF;   // Library ID
  got++;                     // Got to GOT+8
  // Check if the resolve function has been set already...
  if (*got != 0)
    return; // Already set, no need to copy the resolve function again.
  
  uintptr_t resolveLocation = 0;

  // Grab a page to copy the PLT resolve function to.
  // Start at 0x20000000, looking for the next free page.
  /// \todo Change this to use the size of the elf!
  for (uintptr_t i = 0x40000000; i < 0x50000000; i += 0x1000) /// \todo Page size here.
  {
    bool failed = false;
    if (Processor::information().getVirtualAddressSpace().isMapped(reinterpret_cast<void*>(i)))
    {
      failed = true;
      continue;
    }

    resolveLocation = i;
    break;
  }

  if (resolveLocation == 0)
  {
    ERROR("DynamicLinker: nowhere to put resolve function.");
    return;
  }

  physical_uintptr_t physPage = PhysicalMemoryManager::instance().allocatePage();
  bool b = Processor::information().getVirtualAddressSpace().map(physPage,
                                                                 reinterpret_cast<void*> (resolveLocation),
                                                                 VirtualAddressSpace::Write);

  if (!b)
  {
    ERROR("DynamicLinker: Could not map resolve function.");
  }

  // Memcpy over the resolve function into the user address space.
  // resolveSymbol is an ASM function, defined in ./asm-$ARCH.s
  memcpy(reinterpret_cast<uint8_t*> (resolveLocation), reinterpret_cast<uint8_t*> (&::resolveSymbol), 0x1000); /// \todo Page size here.

  *got = resolveLocation;
}

uintptr_t DynamicLinker::resolvePltSymbol(uintptr_t libraryId, uintptr_t symIdx)
{
  // Find the correct ELF to patch.
  Elf32 *pElf = 0;
  uintptr_t loadBase = 0;
  
  if (libraryId == 0)
    pElf = m_ProcessElfs.lookup(Processor::information().getCurrentThread()->getParent());
  else
  {
    // Library search.
    // Grab the list of loaded shared objects for this process.
    List<SharedObject*> *pList = m_ProcessObjects.lookup(Processor::information().getCurrentThread()->getParent());
  
    // Look through the shared object list.
    for (List<SharedObject*>::Iterator it = pList->begin();
        it != pList->end();
        it++)
    {
      pElf = findElf(libraryId, *it, loadBase);
      if (pElf != 0) break;
    }
  }
  
  pElf->setLoadBase(loadBase);
  uintptr_t result = pElf->applySpecificRelocation(symIdx, &resolve);
  
  return result;
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
    if (ptr=pSo->addresses.lookup(pProcess))
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

  setGot(pElf, 0);
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
    if (ptr=pSo->addresses.lookup(Processor::information().getCurrentThread()->getParent()))
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
