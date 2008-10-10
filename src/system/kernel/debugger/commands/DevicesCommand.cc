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

#include "DevicesCommand.h"
#include <utilities/utility.h>
#include <DebuggerIO.h>
#include <processor/Processor.h>
#include <Log.h>
#include <machine/Machine.h>
#include <KernelElf.h>

DevicesCommand::DevicesCommand()
  : DebuggerCommand()
{
}

DevicesCommand::~DevicesCommand()
{
}

void DevicesCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool DevicesCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
  // Let's enter 'raw' screen mode.
  pScreen->disableCli();

  // Work out where the split will be between the tree and the info.
  // Give the tree 3/4 of the available lines.
  int nLines = ((pScreen->getHeight()-2) * 3) / 4;

  static DeviceTree tree;
  tree.move(0, 1);
  tree.resize(pScreen->getWidth(), nLines);
  tree.setScrollKeys('o', 'p');

  static DeviceInfo info;
  info.move(0, nLines+2);
  info.resize(pScreen->getWidth(), pScreen->getHeight()-nLines-3);
  info.setScrollKeys('j', 'k');

  pScreen->disableRefreshes();
  drawBackground(nLines, pScreen);
  tree.refresh(pScreen);
  info.refresh(pScreen);
  pScreen->enableRefreshes();


  // Here we enter our main runloop.
  bool bContinue = true;
  while (bContinue)
  {
    char c;
    while( (c=pScreen->getChar()) == 0)
      ;
    if (c == 'q')
      break;
    else if (c == 'o')
    {
      if (tree.m_Line > 0)
        tree.m_Line--;
      tree.centreOn(tree.m_Line);
      tree.refresh(pScreen);
      info.setDevice(tree.getDevForIndex(tree.m_Line));
      info.refresh(pScreen);
    }
    else if (c == 'p')
    {
      if (tree.m_Line < tree.getLineCount()-1)
        tree.m_Line++;
      tree.centreOn(tree.m_Line);
      tree.refresh(pScreen);
      info.setDevice(tree.getDevForIndex(tree.m_Line));
      info.refresh(pScreen);
    }
    else if (c == 'j')
    {
      info.scroll(-1);
      info.refresh(pScreen);
    }
    else if (c == 'k')
    {
      info.scroll(1);
      info.refresh(pScreen);
    }
    else if (c == 'd')
    {
      // Dump.
      String str;
      info.getDevice()->dump(str);
      drawBackground(0, pScreen);
      pScreen->drawString("Querying device - press any key to exit. ", 1, 0, DebuggerIO::White, DebuggerIO::Black);
      pScreen->drawString(str, 2, 0, DebuggerIO::LightGrey, DebuggerIO::Black);
      while( (c=pScreen->getChar()) == 0)
        ;
      drawBackground(nLines, pScreen);
      tree.refresh(pScreen);
      info.refresh(pScreen);
    }
  }
  
  // Let's enter CLI screen mode again.
  pScreen->enableCli();
  return true; // Return control to the debugger.
}

void DevicesCommand::drawBackground(size_t nLines, DebuggerIO *pScreen)
{
  // Destroy all text.
  pScreen->cls();

  // Clear the top and bottom status lines.
  pScreen->drawHorizontalLine(' ', 0, 0, pScreen->getWidth()-1,
                              DebuggerIO::White, DebuggerIO::Green);
  pScreen->drawHorizontalLine(' ', pScreen->getHeight()-1, 0, pScreen->getWidth()-1,
                              DebuggerIO::White, DebuggerIO::Green);
  // Write the correct text in the upper status line.
  pScreen->drawString("Pedigree debugger - Device tree", 0, 0, DebuggerIO::White, DebuggerIO::Green);
  // Write some helper text in the lower status line.
  pScreen->drawString("", 
                      pScreen->getHeight()-1, 0, DebuggerIO::White, DebuggerIO::Green);

  pScreen->drawHorizontalLine('-', nLines+1, 0, pScreen->getWidth(), DebuggerIO::DarkGrey, DebuggerIO::Black);
}

DevicesCommand::DeviceTree::DeviceTree() :
  m_Line(0), m_LinearTree()
{
  // Create the (flattened) device tree, so we can look up the index of any particular device.

  // Get the first device.
  Device *device = &Device::root();
  
  // Start the depth-first search.
  probeDev(device);
}

void DevicesCommand::DeviceTree::probeDev(Device *pDev)
{
  for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
  {
    m_LinearTree.pushBack(pDev->getChild(i));
    probeDev(pDev->getChild(i));
  }
}

const char *DevicesCommand::DeviceTree::getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  NormalStaticString str;

  // Grab the device to describe.
  Device *pDev = m_LinearTree[index];

  // Set the foreground colour.
  colour = DebuggerIO::Yellow;
  
  // If we're the selected line, add our background colour.
  if (m_Line == index)
    bgColour = DebuggerIO::Blue;

  // How many parents does this node have? that will determine how many |'s to draw.
  while (pDev->getParent() != 0)
  {
    pDev = pDev->getParent();
    str += " ";
  }
  str += "- ";

  return str;
}

const char *DevicesCommand::DeviceTree::getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  static LargeStaticString str;

  // Grab the device to describe.
  Device *pDev = m_LinearTree[index];

  // Set the foreground colour.
  colour = DebuggerIO::White;

  String str2;
  pDev->getName(str2);
  str += str2;
  str += "(";
  str += pDev->getSpecificType();
  str += ")";

  // Calculate the column offset.
  // How many parents does this node have? that will determine how many |'s to draw.
  colOffset = 0;
  while (pDev->getParent() != 0)
  {
    pDev = pDev->getParent();
    colOffset++;
  }
  colOffset += 2; // For '- '

  // If we're selected, add our background colour and pad to the end-of-line.
  if (m_Line == index)
  {
    bgColour = DebuggerIO::Blue;
  }
  str.pad(m_width-colOffset);

  return str;
}

size_t DevicesCommand::DeviceTree::getLineCount()
{
  return m_LinearTree.count();
}

Device *DevicesCommand::DeviceTree::getDevForIndex(size_t index)
{
  return m_LinearTree[index];
}

DevicesCommand::DeviceInfo::DeviceInfo() : m_pDev(0)
{
}

void DevicesCommand::DeviceInfo::setDevice(Device *dev)
{
  m_pDev = dev;
}

const char *DevicesCommand::DeviceInfo::getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  colour = DebuggerIO::Yellow;
  switch (index)
  {
    case 0: return "Name";
    case 1: return "(Abstract) type";
    case 2: return "(Specific) type";
    case 3: return "Interrupt";
    case 4: return "Openfirmware Handle";
    case 5: return "Addresses";
    default: return "";
  };
}

const char *DevicesCommand::DeviceInfo::getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  if (!m_pDev) return "";
  static LargeStaticString str;
  colOffset = 40;
  switch (index)
  {
    case 0:
    {
      String str2;
      m_pDev->getName(str2);
      str += str2;
      break;
    }
    case 1:
      str += m_pDev->getType();
      break;
    case 2:
      str += m_pDev->getSpecificType();
      break;
    case 3:
      str += m_pDev->getInterruptNumber();
      break;
    case 4:
#ifdef OPENFIRMWARE
      str.append(reinterpret_cast<uintptr_t> (m_pDev->getOFHandle()), 16);
      break;
#else
      str += "Not applicable";
      break;
#endif
    default:
    {
      unsigned int i = index-5;
      if (i >= m_pDev->addresses().count())
        break;
      Device::Address *address = m_pDev->addresses()[i];
      str += address->m_Name;
      if (address->m_IsIoSpace)
        str += " (IO) @ ";
      else
        str += " (MEM) @ ";
      str.append(address->m_Address, 16);
      str += "-";
      str.append(address->m_Address+address->m_Size, 16);
      break;
    }
  };
  return str;
}

size_t DevicesCommand::DeviceInfo::getLineCount()
{
  if (m_pDev)
    return 5+m_pDev->addresses().count();
  else
    return 5;
}
