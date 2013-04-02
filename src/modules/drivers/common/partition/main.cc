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

#include <Module.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <machine/Device.h>
#include <machine/Disk.h>
#include "msdos.h"
#include "apple.h"
#include <Log.h>

#include <ServiceManager.h>
#include <ServiceFeatures.h>

#include "Partition.h"
#include "PartitionService.h"

static Service *pService = 0;
static ServiceFeatures *pFeatures = 0;

bool probeDevice(Disk *pDev)
{
  // Does the disk have an MS-DOS partition table?
  if (msdosProbeDisk(pDev))
    return true;

  // No? how about an Apple_Map?
  if (appleProbeDisk(pDev))
    return true;

  // Oh well, better luck next time.
  return false;
}

void searchNode(Device *pDev)
{
  for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
  {
    Device *pChild = pDev->getChild(i);
    // Is this a disk?
    String mname;
    pChild->getName(mname);

    bool hasPartitions = false;
    if (pChild->getType() == Device::Disk)
    {
      // Check that none of its children are Partitions (in which case we've probed this before!)
      for (unsigned int i = 0; i < pChild->getNumChildren(); i++)
      {
        String name;
        pDev->getChild(i)->getName(name);
        if (!strcmp(name, "msdos-partition") || !strcmp(name, "apple-partition"))
        {
          hasPartitions = true;
          break;
        }
      }
      if (!hasPartitions)
        hasPartitions = probeDevice(static_cast<Disk*> (pChild));
    }
    // Recurse, if we didn't find any partitions.
    if (!hasPartitions)
      searchNode(pChild);
  }
}

bool PartitionService::serve(ServiceFeatures::Type type, void *pData, size_t dataLen)
{
    // Correct type?
    if(pFeatures->provides(type))
    {
        // We only provide Touch services
        if(type & ServiceFeatures::touch)
        {
            Disk *pDisk = static_cast<Disk *>(pData);
            return probeDevice(pDisk);
        }
    }

    // Not provided by us, fail!
    return false;
}

static void entry()
{
    // Install the Partition Service
    pService = new PartitionService;
    pFeatures = new ServiceFeatures;
    pFeatures->add(ServiceFeatures::touch);
    ServiceManager::instance().addService(String("partition"), pService, pFeatures);

    // Walk the device tree looking for disks that don't have "partition" children.
    Device *pDev = &Device::root();
    searchNode(pDev);
}

static void exit()
{
    ServiceManager::instance().removeService(String("partition"));
    delete pService;
    delete pFeatures;
}

#ifndef ARM_COMMON // No ATA controller
MODULE_INFO("partition", &entry, &exit, "ata");
#else
MODULE_INFO("partition", &entry, &exit);
#endif
