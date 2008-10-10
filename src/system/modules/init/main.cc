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

#include <Log.h>
#include <vfs/VFS.h>
#include <ext2/Ext2Filesystem.h>
#include <machine/Device.h>
#include <machine/Disk.h>
#include <Module.h>
#include <processor/Processor.h>
#include <Elf32.h>
#include <process/Thread.h>
#include <process/Process.h>
#include <process/Scheduler.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <machine/Machine.h>
#include "../../kernel/core/BootIO.h"

extern BootIO bootIO;

static bool probeDisk(Disk *pDisk)
{
  String alias; // Null - gets assigned by the filesystem.
  if (VFS::instance().mount(pDisk, alias))
  {
    NOTICE("Mounted " << alias << " successfully as root.");
    VFS::instance().addAlias(alias, String("root"));
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
      if (probeDisk(static_cast<Disk*> (pChild)) ) return true;
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
  // Mount all available filesystems.
  findDisks(&Device::root());

  HugeStaticString str;
  str += "Loading init program (root:/applications/shell)\n";
  bootIO.write(str, BootIO::White, BootIO::Black);
  str.clear();

  // Load initial program.
  File init = VFS::instance().find(String("root:/applications/shell"));
  if (!init.isValid())
  {
    ERROR("Unable to load init program!");
    return;
  }

  uint8_t *buffer = new uint8_t[init.getSize()];
  init.read(0, init.getSize(), reinterpret_cast<uintptr_t>(buffer));

  Machine::instance().getKeyboard()->setDebugState(false);
  Process::create(buffer, init.getSize(), "init");
}

void destroy()
{
}

const char *g_pModuleName  = "init";
ModuleEntry g_pModuleEntry = &init;
ModuleExit  g_pModuleExit  = &destroy;
const char *g_pDepends[] = {"VFS", "ext2", "posix", "partition", "TUI", 0};
