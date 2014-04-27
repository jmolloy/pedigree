/*
 * 
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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
#include <machine/Machine.h>

SlamCommand g_SlamCommand;

SlamCommand::SlamCommand()
    : DebuggerCommand(), Scrollable(), m_Tree(), m_It(), m_nLines(0), m_nIdx(0), m_Lock(false)
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
  bool bs = m_Lock;
  m_Lock = true;

  // How many lines do we have?
  m_nLines = NUM_SLAM_BT_FRAMES+1;


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
  pScreen->drawString("Pedigree debugger - Slam allocation resolver",
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
  pScreen->drawString("q: Quit. c: Clean. d: Dump to serial. enter: Next allocation.",
                      pScreen->getHeight()-1, 0, DebuggerIO::White, DebuggerIO::Green);
  pScreen->drawString("q", pScreen->getHeight()-1, 0, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("c", pScreen->getHeight()-1, 9, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("d", pScreen->getHeight()-1, 19, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("enter", pScreen->getHeight()-1, 38, DebuggerIO::Yellow, DebuggerIO::Green);

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
    else if (c == 'c')
    {
        clean();
        m_It = m_Tree.begin();
        m_nIdx = 0;
    }
    else if (c == 'd')
    {
        Machine::instance().getSerial(0)->write ("AllocDump {\n");
        for(m_It = m_Tree.begin(); m_It != m_Tree.end(); m_It++)
        {
            SlamAllocation *pA = reinterpret_cast<SlamAllocation*>(m_It.value());
            StaticString<512> str;
            str.clear();
            str += "Alloc {\nBacktrace [";
            for (int i = 0; i < NUM_SLAM_BT_FRAMES; i++)
            {
                str += "0x";
                str.append(pA->bt[i], 16);
                str += "=\\";
                uintptr_t symStart = 0;
                const char *pSym = KernelElf::instance().globalLookupSymbol(pA->bt[i], &symStart);
                if (pSym == 0)
                    str.append(pA->bt[i], 16);
                else
                {
                    LargeStaticString sym(pSym);
                    static symbol_t symbol;
                    demangle(sym, &symbol);
                    str += static_cast<const char*>(symbol.name);
                }
                str += "=\\, ";
            }
            str += "]\nNum ";
            str += pA->n;
            str += "\nSz ";
            str += pA->size;
            str += "\n}\n";
            Machine::instance().getSerial(0)->write (str);
        }
        Machine::instance().getSerial(0)->write ("}\n");
    }
    else if (c == 'q')
      bStop = true;
  }

  // HACK:: Serial connections will fill the screen with the last background colour used.
  //        Here we write a space with black background so the CLI screen doesn't get filled
  //        by some random colour!
  pScreen->drawString(" ", 1, 0, DebuggerIO::White, DebuggerIO::Black);
  pScreen->enableCli();
  m_Lock = bs;
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
      Line += ") Size: ";
      if(pA->size >= 1024*1024)
      {
          Line += (pA->size/(1024*1024));
          Line += "MB";
      }
      else if(pA->size >= 1024)
      {
          Line += (pA->size/1024);
          Line += "KB";
      }
      else
      {
          Line += pA->size;
          Line += "B";
      }
      return Line;
  }
  index--;

  colour = DebuggerIO::White;
  uintptr_t symStart = 0;

  const char *pSym = KernelElf::instance().globalLookupSymbol(pA->bt[index], &symStart);
  if (pSym == 0)
  {
      Line.append(pA->bt[index], 16);
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

void SlamCommand::addAllocation(uintptr_t *backtrace, size_t requested)
{
    if(m_Lock)
        return;
    m_Lock = true;

    uintptr_t accum = 0;

    // Checksum it
    for (int i = 0; i < NUM_SLAM_BT_FRAMES; i++)
        accum ^= backtrace[i];

    // Lookup the checksum.
    SlamAllocation *pOther = m_Tree.lookup(accum);
    if (!pOther)
    {
        SlamAllocation *pAlloc = new SlamAllocation;
        memcpy(&pAlloc->bt, backtrace, NUM_SLAM_BT_FRAMES*sizeof(uintptr_t));
        pAlloc->n = 1;
        pAlloc->size = requested;
        m_Tree.insert(accum, pAlloc);
    }
    else
    {
        pOther->size += requested;
        pOther->n ++;
    }
    m_Lock = false;
}

void SlamCommand::removeAllocation(uintptr_t *backtrace, size_t requested)
{
    if(m_Lock)
        return;
    m_Lock = true;

    uintptr_t accum = 0;

    // Checksum it
    for (int i = 0; i < NUM_SLAM_BT_FRAMES; i++)
        accum ^= backtrace[i];

    // Lookup the checksum.
    SlamAllocation *pAlloc = m_Tree.lookup(accum);
    if (pAlloc)
    {
        if(pAlloc->n == 1)
            m_Tree.remove(accum);
        else
            pAlloc->n--;
    }
    m_Lock = false;
}
