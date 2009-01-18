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
#include <Module.h>
#include <processor/types.h>
#include <machine/Machine.h>
#include <machine/Device.h>
#include <machine/Bus.h>
#include <processor/IoPort.h>
#include <utilities/utility.h>
#include "pci_list.h"

#define CONFIG_ADDRESS 0
#define CONFIG_DATA    4

#define MAX_BUS 1

IoPort configSpace("PCI config space");

struct ConfigAddress
{
  uint32_t always0  : 2;
  uint32_t offset   : 6;
  uint32_t function : 3;
  uint32_t device   : 5;
  uint32_t bus      : 8;
  uint32_t reserved : 7;
  uint32_t enable   : 1;
};

struct ConfigSpace
{
  uint16_t vendor;
  uint16_t device;
  uint16_t command;
  uint16_t status;
  uint16_t revision;
  uint8_t  subclass;
  uint8_t  class_code;
  uint8_t  cache_line_size;
  uint8_t  latency_timer;
  uint8_t  header_type;
  uint8_t  bist;
  uint32_t bar[6];
  uint32_t cardbus_pointer;
  uint16_t subsys_vendor;
  uint16_t subsys_id;
  uint32_t rom_base_address;
  uint32_t reserved0;
  uint32_t reserved1;
  uint8_t  interrupt_line;
  uint8_t  interrupt_pin;
  uint8_t  min_grant;
  uint8_t  max_latency;
} __attribute__((packed));

uint32_t readConfig (ConfigAddress addr)
{
  // Ensure the enable bit is set.
  addr.enable = 1;
  // And that the always0 bits are zero - duh.
  addr.always0 = 0;
  // Send the address.
  uint32_t a = *reinterpret_cast<uint32_t*>(&addr);

  configSpace.write32(a, CONFIG_ADDRESS);
  // And read back the result.
  return configSpace.read32(CONFIG_DATA);
}

uint32_t writeConfig (ConfigAddress addr, uint32_t value)
{
  // Ensure the enable bit is set.
  addr.enable = 1;
  // And that the always0 bits are zero - duh.
  addr.always0 = 0;
  // Send the address.
  uint32_t a = *reinterpret_cast<uint32_t*>(&addr);

  configSpace.write32(a, CONFIG_ADDRESS);
  configSpace.write32(CONFIG_DATA);
}

void readConfigSpace (ConfigAddress addr, ConfigSpace *pCs)
{
  uint32_t *pCs32 = reinterpret_cast<uint32_t*> (pCs);
  for (int i = 0; i < sizeof(ConfigSpace)/4; i++)
  {
    addr.offset = i;
    pCs32[i] = readConfig(addr);
  }
}

const char *getVendor (uint16_t vendor)
{
  for (int i = 0; i < PCI_VENTABLE_LEN; i++)
  {
    if (PciVenTable[i].VenId == vendor)
      return PciVenTable[i].VenShort;
  }
}

const char *getDevice (uint16_t vendor, uint16_t device)
{
  for (int i = 0; i < PCI_DEVTABLE_LEN; i++)
  {
    if (PciDevTable[i].VenId == vendor && PciDevTable[i].DevId == device)
      return PciDevTable[i].ChipDesc;
  }
}

void entry()
{
  if (!configSpace.allocate(0xCF8, 8))
  {
    ERROR("PCI: Config space - unable to allocate IO port!");
    return;
  }

  configSpace.write32(0x80000000, CONFIG_ADDRESS);
  if (configSpace.read32(CONFIG_ADDRESS) != 0x80000000)
  {
    ERROR("PCI: Controller not detected.");
    return;
  }

  // For every possible device, perform a probe.
  ConfigAddress ca;
  memset(&ca, 0, sizeof(ca));

  for (int i = 0; i < MAX_BUS; i++)
  {
    // Firstly add the ISA bus.
    char *str = new char[256];
    sprintf(str, "PCI #%d", i);
    Bus *pBus = new Bus(str);
    pBus->setSpecificType(String("pci"));

    for (int j = 0; j < 32; j++)
    {
      for (int k = 0; k < 8; k++)
      {
        ca.bus = i;
        ca.device = j;
        ca.function = k;
        ca.offset = 0;
        uint32_t vendorAndDeviceId = readConfig (ca);

        if ( (vendorAndDeviceId & 0xFFFF) == 0xFFFF || (vendorAndDeviceId & 0xFFFF) == 0 )
          break;

        ConfigSpace cs;
        readConfigSpace(ca, &cs);

        NOTICE("PCI: " << Dec << ca.bus << ":" << ca.device << ":" << ca.function << "\t Vendor:" << Hex << cs.vendor << " Device:" << cs.device);

        Device *pDevice = new Device();
        char c[256];
        sprintf(c, "%s - %s", getDevice(cs.vendor, cs.device), getVendor(cs.vendor));
        pDevice->setSpecificType(String(c));
        pDevice->setPciIdentifiers(cs.class_code, cs.subclass, cs.vendor, cs.device);

        for (int l = 0; l < 6; l++)
        {
          // Write the BAR with FFFFFFFF to discover the size of mapping that the device requires.
          ConfigAddress ca2;
          ca2.bus = i; ca2.device = j; ca2.function = k; ca.offset = 0x10 + l*4;
          writeConfig (ca2, 0xFFFFFFFF);
          uint32_t mask = readConfig (ca2);

          // Now work out how much space is required to fill that mask.
          // Assume it doesn't need 4GB of space...
          uint32_t size = ~mask + 1;

          sprintf(c, "bar%d", l);
          pDevice->addresses().pushBack(new Device::Address(String(c), cs.bar[0]&0xFFFFFFF0, size, (cs.bar[0]&0x1) == 0x1));
        }

        pDevice->setInterruptNumber(cs.interrupt_line);
        pBus->addChild(pDevice);
        pDevice->setParent(pBus);
      }
    }

    // If the bus was actually populated...
    if (pBus->getNumChildren() > 0)
    {
      Device::root().addChild(pBus);
      pBus->setParent(&Device::root());
    }
    else
    {
      delete pBus;
    }
  }
}

void exit()
{
}

MODULE_NAME("pci");
MODULE_ENTRY(&entry);
MODULE_EXIT(&exit);
MODULE_DEPENDS(0);

