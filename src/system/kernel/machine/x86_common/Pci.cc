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
#include <processor/types.h>
#include <machine/Machine.h>
#include <machine/Device.h>
#include <machine/Bus.h>
#include <processor/IoPort.h>
#include <utilities/utility.h>
#include <machine/Pci.h>

#define CONFIG_ADDRESS 0
#define CONFIG_DATA    4

#define MAX_BUS 4

static IoPort configSpace("PCI config space");

union ConfigAddress {
  struct
  {
    uint32_t always0  : 2;
    uint32_t offset   : 6;
    uint32_t function : 3;
    uint32_t device   : 5;
    uint32_t bus      : 8;
    uint32_t reserved : 7;
    uint32_t enable   : 1;
  } __attribute__((packed));
  uint32_t raw;
};

PciBus PciBus::m_Instance;

PciBus::PciBus()
{
}

PciBus::~PciBus()
{
}

void PciBus::initialise()
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
}

uint32_t PciBus::readConfigSpace(Device *pDev, uint8_t offset)
{
    ConfigAddress addr;
    memset(&addr, 0, sizeof(addr));
    addr.offset = offset;
    addr.function = pDev->getPciFunctionNumber();
    addr.device = pDev->getPciDevicePosition();
    addr.bus = pDev->getPciBusPosition();
    addr.enable = 1;

    configSpace.write32(addr.raw, CONFIG_ADDRESS);
    return configSpace.read32(CONFIG_DATA);
}

uint32_t PciBus::readConfigSpace(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    ConfigAddress addr;
    memset(&addr, 0, sizeof(addr));
    addr.offset = offset;
    addr.function = function;
    addr.device = device;
    addr.bus = bus;
    addr.enable = 1;

    configSpace.write32(addr.raw, CONFIG_ADDRESS);
    return configSpace.read32(CONFIG_DATA);
}

void PciBus::writeConfigSpace(Device *pDev, uint8_t offset, uint32_t data)
{
    ConfigAddress addr;
    memset(&addr, 0, sizeof(addr));
    addr.offset = offset;
    addr.function = pDev->getPciFunctionNumber();
    addr.device = pDev->getPciDevicePosition();
    addr.bus = pDev->getPciBusPosition();
    addr.enable = 1;

    configSpace.write32(addr.raw, CONFIG_ADDRESS);
    configSpace.write32(data, CONFIG_DATA);
}

void PciBus::writeConfigSpace(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t data)
{
    ConfigAddress addr;
    memset(&addr, 0, sizeof(addr));
    addr.offset = offset;
    addr.function = function;
    addr.device = device;
    addr.bus = bus;
    addr.enable = 1;

    configSpace.write32(addr.raw, CONFIG_ADDRESS);
    configSpace.write32(data, CONFIG_DATA);
}
