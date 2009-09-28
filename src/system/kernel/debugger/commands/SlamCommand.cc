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

#include "SlamCommand.h"
#include <Log.h>
#include <utilities/utility.h>
#include <DebuggerIO.h>
#include <process/Scheduler.h>
#include <process/Process.h>
#include <linker/KernelElf.h>
#include <utilities/demangle.h>
#include <processor/Processor.h>
#include <Backtrace.h>

SlamCommand g_SlamCommand;

SlamCommand::SlamCommand()
    : DebuggerCommand(), Scrollable(), m_Allocations(), m_nLines(0), m_Tree(),
      m_It(), m_nIdx(0)
{
}

SlamCommand::~SlamCommand()
{
}

void SlamCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool SlamCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{

  // How many lines do we have?
  m_nLines = NUM_BT_FRAMES+1;

  m_Tree.clear();

  for (int i = 0; i < 32; i++)
      SlamAllocator::instance().m_Caches[i].check(true);

  // Perform preprocessing. Horrible O(n^2) algorithm.
  for (Vector<SlamAllocation*>::Iterator it = m_Allocations.begin();
       it != m_Allocations.end();
       it++)
  {
      SlamAllocation *pA = *it;
      // Create a checksum of the backtrace.
      uintptr_t accum = 0;
      for (int i = 0; i < NUM_BT_FRAMES; i++)
          accum ^= pA->ra[i];

      // Along with process ID...
      accum += pA->pid<<16;

      // Lookup the checksum.
      SlamAllocation *pOther = m_Tree.lookup(accum);
      if (pOther == 0)
      {
          pA->n = 1;
          m_Tree.insert(accum, pA);
      }
      else
      {
          pOther->n ++;
      }
  }

  m_It = m_Tree.begin();
  m_nIdx = 0;

  // Let's enter 'raw' screen mode.
  pScreen->disableCli();

  // Initialise the Scrollable class
  move(0, 1);
  resize(pScreen->getWidth(), pScreen->getHeight() - 2);
  setScrollKeys('j', 'k');

  // Clear the top status lines.
  pScreen->drawHorizontalLine(' ',
                              0,
                              0,
                              pScreen->getWidth() - 1,
                              DebuggerIO::White,
                              DebuggerIO::Green);

  // Write the correct text in the upper status line.
  pScreen->drawString("Pedigree debugger - Page allocation resolver",
                      0,
                      0,
                      DebuggerIO::White,
                      DebuggerIO::Green);

  // Clear the bottom status lines.
  // TODO: If we use arrow keys and page up/down keys we actually can remove the status line
  //       because the interface is then intuitive enough imho.
  pScreen->drawHorizontalLine(' ',
                              pScreen->getHeight() - 1,
                              0,
                              pScreen->getWidth() - 1,
                              DebuggerIO::White,
                              DebuggerIO::Green);

  // Write some helper text in the lower status line.
  // TODO FIXME: Drawing this might screw the top status bar
  pScreen->drawString("backspace: Page up. space: Page down. q: Quit. enter: Next allocation",
                      pScreen->getHeight()-1, 0, DebuggerIO::White, DebuggerIO::Green);
  pScreen->drawString("backspace", pScreen->getHeight()-1, 0, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("space", pScreen->getHeight()-1, 20, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("q", pScreen->getHeight()-1, 38, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("enter", pScreen->getHeight()-1, 47, DebuggerIO::Yellow, DebuggerIO::Green);

  // Main loop.
  bool bStop = false;
  bool bReturn = true;
  while(!bStop)
  {
    refresh(pScreen);

    // Wait for input.
    char c = 0;
    while( !(c=pScreen->getChar()) )
      ;

    // TODO: Use arrow keys and page up/down someday
    if (c == 'j')
    {
      scroll(-1);
    }
    else if (c == 'k')
    {
      scroll(1);
    }
    else if (c == ' ')
    {
      scroll(static_cast<ssize_t>(height()));
    }
    else if (c == 0x08)
    {
      scroll(-static_cast<ssize_t>(height()));
    }
    else if (c == '\n' || c == '\r')
    {
        m_nIdx++;
        m_It ++;
        if (m_It == m_Tree.end())
        {
            m_It = m_Tree.begin();
            m_nIdx = 0;
        }
    }
    else if (c == 'q')
      bStop = true;
  }

  m_Allocations.clear();

  // HACK:: Serial connections will fill the screen with the last background colour used.
  //        Here we write a space with black background so the CLI screen doesn't get filled
  //        by some random colour!
  pScreen->drawString(" ", 1, 0, DebuggerIO::White, DebuggerIO::Black);
  pScreen->enableCli();
  return bReturn;
}

const char *SlamCommand::getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  static NormalStaticString Line;
  Line.clear();

  SlamAllocation *pA = reinterpret_cast<SlamAllocation*>(m_It.value());

  bgColour = DebuggerIO::Black;
  if (index == 0)
  {
      colour = DebuggerIO::Yellow;
      Line += pA->n;
      Line += " allocations from this source (";
      Line += m_nIdx;
      Line += "/";
      Line += m_Tree.count();
      Line += ") PID: ";
      Line += pA->pid;
      return Line;
  }
  index--;

  colour = DebuggerIO::White;
  uintptr_t symStart = 0;

  const char *pSym = KernelElf::instance().globalLookupSymbol(pA->ra[index], &symStart);
  if (pSym == 0)
  {
      Line.append(pA->ra[index], 16);
  }
  else
  {
      LargeStaticString sym(pSym);

      Line += "[";
      Line.append(symStart, 16);
      Line += "] ";
      static symbol_t symbol;
      demangle(sym, &symbol);
      Line += static_cast<const char*>(symbol.name);
  }

  return Line;
}
const char *SlamCommand::getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  static LargeStaticString Line;
  Line.clear();

  return Line;
}

size_t SlamCommand::getLineCount()
{
  return m_nLines;
}

void SlamCommand::addAllocation(SlamAllocator::AllocHeader *head)
{
    SlamAllocation *pA = new SlamAllocation;
    memcpy(&pA->ra, &head->backtrace, NUM_BT_FRAMES*sizeof(uintptr_t));

    Process *pP = Processor::information().getCurrentThread()->getParent();
    if (pP)
        pA->pid = pP->getId();
    else
        pA->pid = -1;

    m_Allocations.pushBack(pA);
}
