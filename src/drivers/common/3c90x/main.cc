/*
* Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin, eddyb
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
#include <machine/Network.h>
#include "3Com90x.h"
#include <Log.h>

struct nic
{
  uint16_t vendor, device;
  const char *type;
  const char *desc;
};

static struct nic potential_nics[] = {
/* Original 90x revisions: */
{0x10b7, 0x9000, "3c905-tpo",     "3Com900-TPO"},	/* 10 Base TPO */
{0x10b7, 0x9001, "3c905-t4",      "3Com900-Combo"},	/* 10/100 T4 */
{0x10b7, 0x9050, "3c905-tpo100",  "3Com905-TX"},		/* 100 Base TX / 10/100 TPO */
{0x10b7, 0x9051, "3c905-combo",   "3Com905-T4"},		/* 100 Base T4 / 10 Base Combo */
/* Newer 90xB revisions: */
{0x10b7, 0x9004, "3c905b-tpo",    "3Com900B-TPO"},	/* 10 Base TPO */
{0x10b7, 0x9005, "3c905b-combo",  "3Com900B-Combo"},	/* 10 Base Combo */
{0x10b7, 0x9006, "3c905b-tpb2",   "3Com900B-2/T"},	/* 10 Base TP and Base2 */
{0x10b7, 0x900a, "3c905b-fl",     "3Com900B-FL"},	/* 10 Base FL */
{0x10b7, 0x9055, "3c905b-tpo100", "3Com905B-TX"},	/* 10/100 TPO */
{0x10b7, 0x9056, "3c905b-t4",     "3Com905B-T4"},	/* 10/100 T4 */
{0x10b7, 0x9058, "3c905b-9058",   "3Com905B-9058"},	/* Cyclone 10/100/BNC */
{0x10b7, 0x905a, "3c905b-fx",     "3Com905B-FL"},	/* 100 Base FX / 10 Base FX */
/* Newer 90xC revision: */
{0x10b7, 0x9200, "3c905c-tpo",    "3Com905C-TXM"},	/* 10/100 TPO {3C905C-TXM} */
{0x10b7, 0x9800, "3c980",         "3Com980-Cyclone"},	/* Cyclone */
{0x10b7, 0x9805, "3c9805",        "3Com9805"},		/* Dual Port Server Cyclone */
{0x10b7, 0x7646, "3csoho100-tx",  "3CSOHO100-TX"},	/* Hurricane */
{0x10b7, 0x4500, "3c450",         "3Com450 HomePNA Tornado"},
};

#define NUM_POTENTIAL_NICS (sizeof(potential_nics) / sizeof(potential_nics[0]))

void probeDevice(Network *pDev)
{
    // Create a new RTL8139 node
    Nic3C90x *pCard = new Nic3C90x(pDev);

    // Replace pDev with pRtl8139.
    pCard->setParent(pDev->getParent());
    pDev->getParent()->replaceChild(pDev, pCard);


    // And delete pDev for good measure.
    //  - Deletion not needed now that AtaController(pDev) destroys pDev. See Device::Device(Device *)
    //delete pDev;
}

void searchNode(Device *pDev)
{
    for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
    {
        Device *pChild = pDev->getChild(i);

        for(unsigned int j = 0; j < NUM_POTENTIAL_NICS; j++)
        {
          uint16_t vendor = potential_nics[i].vendor;
          uint16_t device = potential_nics[i].device;
          if((pChild->getPciVendorId() == vendor) && (pChild->getPciDeviceId() == device))
          {
            uintptr_t irq = pChild->getInterruptNumber();
            NOTICE("3C90x [" << potential_nics[i].type << "/" << potential_nics[i].desc << "] found, IRQ = " << irq << ".");

            if(pChild->addresses()[0]->m_IsIoSpace)
                probeDevice(reinterpret_cast<Network*>(pChild));
          }
        }

        // Recurse.
        searchNode(pChild);
    }
}

void entry()
{
    // Walk the device tree looking for controllers that have
    // "control" and "command" addresses.
    Device *pDev = &Device::root();
    searchNode(pDev);
}

void exit()
{

}

MODULE_NAME("rtl8139");
MODULE_ENTRY(&entry);
MODULE_EXIT(&exit);
MODULE_DEPENDS("NetworkStack");
