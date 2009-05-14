/*
 * Copyright (c) 2008 James Molloy, JÃ¶rg PfÃ¤hler, Matthew Iselin
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

#include <Log.h>
#include <vfs/VFS.h>
#include <vfs/Directory.h>
#include <machine/Device.h>
#include <machine/Disk.h>
#include <Module.h>
#include <processor/Processor.h>
#include <linker/Elf.h>
#include <process/Thread.h>
#include <process/Process.h>
#include <process/Scheduler.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <machine/Machine.h>
#include <linker/DynamicLinker.h>
#include <panic.h>

#include "../../kernel/core/BootIO.h"

#include <network/NetworkStack.h>
#include <network/RoutingTable.h>

#include <users/UserManager.h>

extern BootIO bootIO;

void init_stage2();

class RamDir : public Directory
{
private:
  RamDir(const RamDir &);
  RamDir& operator =(const RamDir&);
public:
  RamDir(String name, size_t inode, class Filesystem *pFs, File *pParent) :
    Directory(name, 0, 0, 0, inode, pFs, 0, pParent), m_FileTree()
  {}
  virtual ~RamDir() {};

  uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer)
  {return 0;}
  uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer)
  {return 0;}

  void truncate()
  {}

  virtual void cacheDirectoryContents()
  {}

  virtual bool addEntry(String filename, File *pFile, size_t type)
  {
    // This'll get destroyed in the first write
    uint8_t *newFile = new uint8_t;
    pFile->setInode(reinterpret_cast<uintptr_t>(newFile));
    // m_FileTree.insert(filename, pFile);
    m_Cache.insert(filename, pFile);
    m_bCachePopulated = true;
    return true;
  }

  virtual bool removeEntry(File *pFile)
  {
    uint8_t *buff = reinterpret_cast<uint8_t*>(pFile->getInode());
    if(buff)
      delete [] buff;
    // m_FileTree.remove(pFile->getName());
    m_Cache.remove(pFile->getName());
    return true;
  }

  void fileAttributeChanged()
  {};

private:
  // List of files we have available in this directory
  RadixTree<File*> m_FileTree;
};

class RamFs : public Filesystem
{
public:
  RamFs() : m_pRoot(0)
  {}
  virtual ~RamFs()
  {}

  virtual bool initialise(Disk *pDisk)
  {
    m_pRoot = new RamDir(String(""), 0, this, 0);
    return true;
  }
  virtual File* getRoot()
  {
    return m_pRoot;
  }
  virtual String getVolumeLabel()
  {
    return String("ramfs");
  }
  virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
  {
    if(pFile->isDirectory())
      return 0;

    if(!pFile->getSize())
      return 0;

    size_t nBytes = 0;
    if(location > pFile->getSize())
      return 0;
    else if((location + size) > pFile->getSize())
      nBytes = pFile->getSize() - location;
    else
      nBytes = size;

    if(nBytes == 0)
      return 0;

    uint8_t *dest = reinterpret_cast<uint8_t*>(buffer);
    uint8_t *src = reinterpret_cast<uint8_t*>(pFile->getInode());

    memcpy(dest, src + location, nBytes);

    return nBytes;
  }
  virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
  {
    if(pFile->isDirectory())
      return 0;

    size_t nBytes = size;

    // Reallocate if needed
    if((location + size) > pFile->getSize())
    {
      size_t numExtra = (location + size) - pFile->getSize();
      uint8_t *newBuff = new uint8_t[pFile->getSize() + numExtra];
      delete [] reinterpret_cast<uint8_t*>(pFile->getInode());
      pFile->setInode(reinterpret_cast<uintptr_t>(newBuff));

      pFile->setSize(pFile->getSize() + numExtra);
    }

    uint8_t *dest = reinterpret_cast<uint8_t*>(pFile->getInode());
    uint8_t *src = reinterpret_cast<uint8_t*>(buffer);

    memcpy(dest + location, src, nBytes);

    return nBytes;
  }
  virtual void truncate(File *pFile) {};
  virtual void fileAttributeChanged(File *pFile) {};
  virtual void cacheDirectoryContents(File *pFile) {};

protected:
  virtual bool createFile(File* parent, String filename, uint32_t mask)
  {
    if(!parent->isDirectory())
      return false;

    File *f = new File(filename, 0, 0, 0, 0, this, 0, parent);

    RamDir *p = reinterpret_cast<RamDir*>(parent);
    return p->addEntry(filename, f, 0);
  }
  virtual bool createDirectory(File* parent, String filename)
  {
    return false;
  }
  virtual bool createSymlink(File* parent, String filename, String value)
  {
    return false;
  }
  virtual bool remove(File* parent, File* file)
  {
    if(file->isDirectory())
      return false;

    RamDir *p = reinterpret_cast<RamDir*>(parent);
    return p->removeEntry(file);
  }

  RamFs(const RamFs&);
  void operator =(const RamFs&);

  /** Root filesystem node. */
  File *m_pRoot;
};

static bool probeDisk(Disk *pDisk)
{
  String alias; // Null - gets assigned by the filesystem.
  if (VFS::instance().mount(pDisk, alias))
  {
    // search for the root specifier
    NormalStaticString s;
    s += alias;
    s += ":/.pedigree-root";

    File* f = VFS::instance().find(String(static_cast<const char*>(s)));
    if(f)
    {
      NOTICE("Mounted " << alias << " successfully as root.");
      VFS::instance().addAlias(alias, String("root"));
    }
    else
    {
      NOTICE("Mounted " << alias << ".");
    }
    return false;
  }
  return false;
}

static bool findDisks(Device *pDev)
{
  for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
  {
    Device *pChild = pDev->getChild(i);
    if (pChild->getNumChildren() == 0 && /* Only check leaf nodes. */
        pChild->getType() == Device::Disk)
    {
      if ( probeDisk(static_cast<Disk*> (pChild)) ) return true;
    }
    else
    {
      // Recurse.
      if (findDisks(pChild)) return true;
    }
  }
  return false;
}

void init()
{
  static HugeStaticString str;

  // Setup the RAMFS for the mount table before the real HDDs. Doing it early
  // means if someone happens to have a disk with a volume label that matches
  // "ramfs", we don't end up failing later.
  RamFs *ramFs = new RamFs;
  ramFs->initialise(0);

  // Add it to the VFS
  VFS::instance().addAlias(ramFs, ramFs->getVolumeLabel());

  // Mount all available filesystems.
  if (!findDisks(&Device::root()))
  {
//     FATAL("No disks found!");
  }

  // Build the mount table
  if(!VFS::instance().createFile(String("ramfs:/mount.tab"), 0777, 0))
    FATAL("Couldn't create mount table in the RAMFS");
  File* mountTab = VFS::instance().find(String("ramfs:/mount.tab"));

  List<VFS::Alias*> myAliases = VFS::instance().getAliases();
  NormalStaticString myMounts;
  for(List<VFS::Alias*>::Iterator i = myAliases.begin(); i != myAliases.end(); i++)
  {
    // Check for the mounts upon which we *don't* want Pedigree to be installed on
    String alias = (*i)->alias;
    if(!strncmp(static_cast<const char*>(alias), "PEDIGREEINSTALL", strlen("PEDIGREEINSTALL")))
      continue;
    if(!strncmp(static_cast<const char*>(alias), "root", strlen("root")))
      continue;
    if(!strncmp(static_cast<const char*>(alias), "ramfs", strlen("ramfs")))
      continue;

    myMounts += alias;
    myMounts += '\n';
  }

  // Write to the file
  mountTab->write(0, myMounts.length(), reinterpret_cast<uintptr_t>(static_cast<const char*>(myMounts)));

  mountTab->decreaseRefCount(true);

  NOTICE("Available mounts for install:\n" << String(myMounts));

  // Initialise user/group configuration.
  UserManager::instance().initialise();

  str += "Loading init program (root:/applications/login)\n";
  bootIO.write(str, BootIO::White, BootIO::Black);
  str.clear();

  Machine::instance().getKeyboard()->setDebugState(false);

  // At this point we're uninterruptible, as we're forking.
  Spinlock lock;
  lock.acquire();

  // Create a new process for the init process.
  Process *pProcess = new Process(Processor::information().getCurrentThread()->getParent());
  pProcess->setUser(UserManager::instance().getUser(0));
  pProcess->setGroup(UserManager::instance().getUser(0)->getDefaultGroup());

  pProcess->description().clear();
  pProcess->description().append("init");

  pProcess->setCwd(VFS::instance().find(String("root:/")));

  new Thread(pProcess, reinterpret_cast<Thread::ThreadStartFunc>(&init_stage2), 0x0 /* parameter */);

  lock.release();
}

void destroy()
{
}

void init_stage2()
{
  // Load initial program.
  File* initProg = VFS::instance().find(String("root:/applications/login"));
  if (!initProg)
  {
    FATAL("Unable to load init program!");
    return;
  }

  // That will have forked - we don't want to fork, so clear out all the chaff in the new address space that's not
  // in the kernel address space so we have a clean slate.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  pProcess->getAddressSpace()->revertToKernelAddressSpace();

  DynamicLinker *pLinker = new DynamicLinker();
  pProcess->setLinker(pLinker);

  if (!pLinker->loadProgram(initProg))
  {
      FATAL("Init program failed to load!");
  }

  for (int j = 0; j < 0x20000; j += 0x1000)
  {
    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    bool b = Processor::information().getVirtualAddressSpace().map(phys, reinterpret_cast<void*> (j+0x20000000), VirtualAddressSpace::Write);
    if (!b)
      WARNING("map() failed in init");
  }

  // Alrighty - lets create a new thread for this program - -8 as PPC assumes the previous stack frame is available...
  new Thread(pProcess, reinterpret_cast<Thread::ThreadStartFunc>(pLinker->getProgramElf()->getEntryPoint()), 0x0 /* parameter */,  reinterpret_cast<void*>(0x20020000-8) /* Stack */);

//  Processor::setInterrupts(true);

  // Thread exit.
//  while (1) {Scheduler::instance().yield();}
}

MODULE_NAME("init");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
#ifdef X86_COMMON
MODULE_DEPENDS("VFS", "ext2", "posix", "partition", "TUI", "linker", "vbe", "NetworkStack", "users");
#elif PPC_COMMON
MODULE_DEPENDS("VFS", "ext2", "posix", "partition", "TUI", "linker", "NetworkStack", "users");
#endif

