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

#include "apple.h"
#include <processor/Processor.h>
#include <Log.h>
#include "Partition.h"


bool appleProbeDisk(Disk *pDisk)
{
  // Read the second sector (512 bytes) of the disk into a buffer.
  uint8_t buffer[512];
  if (pDisk->read(512ULL, 512ULL, reinterpret_cast<uint32_t> (buffer)) != 512)
  {
    WARNING("Disk read failure during partition table search.");
    return false;
  }

  ApplePartitionMap *pMap = reinterpret_cast<ApplePartitionMap*> (buffer);

  String diskName;
  pDisk->getName(diskName);

  // Check for the magic signature.
  if (pMap->pmSig != APPLE_PM_SIG)
  {
    NOTICE("Apple partition map not found on disk " << diskName);
    return false;
  }
  
  NOTICE("Apple partition map found on disk " << diskName);

  // Cache the number of partition table entries.
  size_t nEntries = pMap->pmMapBlkCnt;
  for (int i = 0; i < nEntries; i++)
  {
    if (i > 0) // We don't need to load anything in for the first pmap - already done!
    {
      if (pDisk->read(512ULL+i*512ULL, 512ULL, reinterpret_cast<uint32_t> (buffer)) != 512)
      {
        WARNING("Disk read failure during partition table recognition.");
        return false;
      }
      pMap = reinterpret_cast<ApplePartitionMap*> (buffer);
    }

    NOTICE("Detected partition '" << pMap->pmPartName << "', type '" << pMap->pmParType << "'");

    // Create a partition object.
    Partition *pObj = new Partition(String(pMap->pmParType),
                                    static_cast<uint64_t>(pMap->pmPyPartStart)*512ULL,
                                    static_cast<uint64_t>(pMap->pmPartBlkCnt)*512ULL);
    pObj->setParent(static_cast<Device*> (pDisk));
    pDisk->addChild(static_cast<Device*> (pObj));
    
  }
  return true;
}
