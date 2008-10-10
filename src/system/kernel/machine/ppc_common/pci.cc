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

#include <machine/ppc_common/pci.h>
#include <Log.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <machine/Device.h>
#include <machine/openfirmware/Device.h>

/** The type of one entry in the "assigned-addresses" property. */
typedef struct
{
  uint32_t n : 1; ///< Non-relocatable.
  uint32_t p : 1;
  uint32_t t : 1;
  uint32_t zero : 3; ///< Always zero.
  uint32_t s : 2; ///< Address space identifier - Memory/IO.
  uint32_t b : 8; ///< Bus ID.
  uint32_t d : 5; ///< Device ID.
  uint32_t f : 3; ///< Function number.
  uint32_t r : 8; ///< Register index - BAR offset.
  uint32_t address_hi; ///< High 32 bits of address. Unused.
  uint32_t address_lo; ///< Low 32 bits of assigned address.
  uint32_t size_hi; ///< High 32 bits of size. Unused.
  uint32_t size_lo; ///< Low 32 bits of size.
} AssignedAddress;

/** The type of one entry in the "range" property, mapping child address
    spaces to parent address spaces. */
typedef struct
{
  uint32_t child_hi; ///< Holds the address space type, corresponding to a "assigned address" entry.
  uint32_t child_lo; ///< 32-bit child range start address.
  uint32_t parent_hi; ///< Unused.
  uint32_t parent_lo; ///< 32-bit parent range start address.
  uint32_t size_hi; ///< Unused.
  uint32_t size_lo; ///< Size of the range, in bytes.
} Range;

static void calculateAddresses(Device *node)
{
  // Firstly grab the contents of the "assigned-addresses" property.
  OFDevice dev (node->getOFHandle());
  AssignedAddress addresses[16];
  ssize_t len = dev.getProperty("assigned-addresses", addresses, sizeof(AssignedAddress)*16);

  if (len != -1) // No addresses assigned!
  {
    // Iterate through each address entry.
    for (int i = 0; i < len/sizeof(AssignedAddress); i++)
    {
      // We need to work our way back up the tree, applying any "range" properties we may encounter (if they apply to us).
      // Get a mask for our address space ('ss').
      uint32_t addressSpaceMask = addresses[i].s << 24;

      Device *parent = node->getParent();
      while (parent)
      {
        OFDevice parentDev (parent->getOFHandle());
        Range ranges[16];
        int len2 = parentDev.getProperty("ranges", ranges, sizeof(Range)*16);
        if (len2 == -1)
        {
          // No range property - try the next parent.
          parent = parent->getParent();
          continue;
        }

        // Iterate over every range.
        for (int j = 0; j < len2/sizeof(Range); j++)
        {
          // Is this range descriptor referring to teh same address space as we are?
          if ( (ranges[j].child_hi & addressSpaceMask) == 0 )
            continue;

          // Are we in its range?
          if ( (ranges[j].child_lo <= addresses[i].address_lo) &&
               ((ranges[j].child_lo+ranges[j].size_lo) > addresses[i].address_lo) )
          {
            // In which case, apply the mapping.
            addresses[i].address_lo -= ranges[j].child_lo;
            addresses[i].address_lo += ranges[j].parent_lo;
            break;
          }
        }
        parent = parent->getParent();
      }
      const char *barStr;
      switch (addresses[i].r)
      {
        case 0x10: barStr = "bar0"; break;
        case 0x14: barStr = "bar1"; break;
        case 0x18: barStr = "bar2"; break;
        case 0x1c: barStr = "bar3"; break;
        case 0x20: barStr = "bar4"; break;
        case 0x24: barStr = "bar5"; break;
        default: barStr = "unknown bar"; break;
      }
      node->addresses().pushBack(
        new Device::Address(String(barStr),
                            addresses[i].address_lo,
                            addresses[i].size_lo,
                            false /* Always Memory for PPC */));
    }
  }
}

static void searchNode(Device *pDev)
{
  for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
  {
    Device *pChild = pDev->getChild(i);
    if (!strcmp(pChild->getSpecificType(), "pci"))
    {
      // We've found a pci device. Calculate addresses for it and all
      // its children.
      calculateAddresses(pChild);
      for (unsigned int j = 0; j < pChild->getNumChildren(); j++)
      {
        Device *pGrandChild = pChild->getChild(j);
        calculateAddresses(pGrandChild);
      }
    }
    // Recurse.
    searchNode(pChild);
  }
}

void initialisePci()
{
  Device *pDev = &Device::root();
  searchNode(pDev);
}
