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

#include <processor/types.h>
#include <machine/openfirmware/Device.h>
#include <machine/Device.h>
#include <panic.h>
#include <Log.h>
#include <processor/Processor.h>

/// Given a device node, attempts to find the interrupt number attached to it.
size_t resolveInterruptNumber(Device *pDev)
{
  OFDevice ofDev (pDev->getOFHandle());

  uint32_t interrupt_specifier;
  uint32_t sense;
  uint32_t buf[2];
  int ret = ofDev.getProperty("interrupts", reinterpret_cast<OFParam>(buf), 8);

  if (ret == -1)
    return static_cast<size_t>(-1); // No interrupts property found!

  interrupt_specifier = buf[0];
  sense = buf[1];

  uint32_t unit_address = reinterpret_cast<uint32_t>(ofDev.getProperty("reg"));

  if (pDev->getParent() == 0)
    panic("What to do?");

  NormalStaticString type;
  ofDev.getProperty("name", type);

  OFHandle this_node;
  if (ofDev.propertyExists("interrupt-parent"))
  {
    this_node = ofDev.getProperty("interrupt-parent");
  }
  else
  {
    this_node = pDev->getParent()->getOFHandle();
  }
  while (this_node != 0)
  {
    OFDevice dev (this_node);
    OFHandle parent_node = 0;

    // Interrupt controller?
    if (dev.propertyExists("interrupt-controller"))
    {
      if (dev.propertyExists("interrupt-parent"))
      {
        parent_node = dev.getProperty("interrupt-parent");
        unit_address = reinterpret_cast<uint32_t>(dev.getProperty("reg"));

        interrupt_specifier = reinterpret_cast<uint32_t>(dev.getProperty("interrupts"));
        this_node = parent_node;
      }
      else // This is the root node, we're done.
      {
        return interrupt_specifier;
      }
    }
    // Not "interrupt-controller"
    else if (dev.propertyExists("interrupt-map"))
    {
      // We have a mapping to perform.
      size_t mapLength = dev.getPropertyLength("interrupt-map");
      uint32_t *map = new uint32_t[mapLength/4];
      dev.getProperty("interrupt-map", reinterpret_cast<OFParam>(map), mapLength);

      uint32_t address_cells = 0;
      if (dev.propertyExists("#address-cells"))
      {
        address_cells = reinterpret_cast<uint32_t>(dev.getProperty("#address-cells"));
      }
      
      // Has a mask?
      if (dev.propertyExists("interrupt-map-mask"))
      {
        size_t maskLength = dev.getPropertyLength("interrupt-map-mask");
        uint32_t *mask = new uint32_t[maskLength/4];
        dev.getProperty("interrupt-map-mask", reinterpret_cast<OFParam>(mask), maskLength);

        unit_address &= mask[0];
        interrupt_specifier &= mask[address_cells];
        delete [] mask;
      }

      size_t i = 0;
      parent_node = 0;
      while (i < (mapLength/4))
      {
        // Might have stumbled on a "sense" cell - 0 or 1?
        if (map[i] == 0 || map[i] == 1)
          i++; // Step over.
        if (address_cells > 0)
        {
          if (map[i] != unit_address)
          {
            i += address_cells;
            i += 1; // Add child interrupt specifier.
            i += 1; // Add parent node.
            i += 1; // Add parent interrupt specifier
            continue;
          }
          else
          {
            i += address_cells;
          }
        }

        if (map[i] != interrupt_specifier)
        {
          i += 1; // Add child interrupt specifier.
          i += 1; // Add parent node.
          i += 1; // Add parent interrupt specifier
          continue;
        }
        else
        {
          i++;
        }
        
        parent_node = reinterpret_cast<OFHandle> (map[i++]);
        interrupt_specifier = map[i++];
        sense = map[i++];
        this_node = parent_node;
        break;
      }
      delete [] map;
      if (parent_node == 0)
      {
        panic("FAIL!");
      }
    }

    // No "interrupt-map" table
    else if (dev.propertyExists("interrupt-parent"))
    {
      this_node = dev.getProperty("interrupt-parent");
    }
    // No "interrupt-parent" property.
    else
    {
      this_node = dev.getParent();
      if (this_node == 0)
        return static_cast<size_t>(-1);
    }

  }
  return interrupt_specifier;
}
