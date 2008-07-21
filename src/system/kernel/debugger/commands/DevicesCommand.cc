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
      tree.scroll(-1);
      tree.refresh(pScreen);
      info.setDevice(tree.getLine());
      info.refresh(pScreen);
    }
    else if (c == 'p')
    {
      tree.scroll(1);
      tree.refresh(pScreen);
      info.setDevice(tree.getLine());
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

DevicesCommand::DeviceTree::DeviceTree()
{
  
}

const char *DevicesCommand::DeviceTree::getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
}

const char *DevicesCommand::DeviceTree::getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
}

size_t DevicesCommand::DeviceTree::getLineCount()
{
}

DevicesCommand::DeviceInfo::DeviceInfo()
{
}

void DevicesCommand::DeviceInfo::setDevice(int n)
{
}

const char *DevicesCommand::DeviceInfo::getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
}

const char *DevicesCommand::DeviceInfo::getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
}

size_t DevicesCommand::DeviceInfo::getLineCount()
{
}
