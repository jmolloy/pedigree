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

#if defined(THREADS)

#include "ThreadsCommand.h"
#include <Log.h>
#include <utilities/utility.h>
#include <DebuggerIO.h>
#include <process/Scheduler.h>
#include <process/Process.h>
#include <KernelElf.h>
#include <utilities/demangle.h>
#include <process/initialiseMultitasking.h>
#include <processor/Processor.h>

ThreadsCommand::ThreadsCommand()
  : DebuggerCommand(), Scrollable(), m_SelectedLine(0), m_nLines(0)
{
}

ThreadsCommand::~ThreadsCommand()
{
}

void ThreadsCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool ThreadsCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
  // How many lines do we have?
  m_nLines = 0;
  for (size_t i = 0; i < Scheduler::instance().getNumProcesses(); i++)
  {
    m_nLines++; // For the process.
    m_nLines += Scheduler::instance().getProcess(i)->getNumThreads();
  }
  
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
  pScreen->drawString("Pedigree debugger - Thread selector",
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
  pScreen->drawString("backspace: Page up. space: Page down. q: Quit. enter: Switch to thread", 
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
      if (static_cast<ssize_t>(m_SelectedLine)-1 >= 0)
        m_SelectedLine--;
    }
    else if (c == 'k')
    {
      scroll(1);
      if (m_SelectedLine+1 < getLineCount())
        m_SelectedLine++;
    }
    else if (c == ' ')
    {
      scroll(static_cast<ssize_t>(height()));
      if (m_SelectedLine+height() < getLineCount())
        m_SelectedLine += height();
      else
        m_SelectedLine = getLineCount()-1;
    }
    else if (c == 0x08)
    {
      scroll(-static_cast<ssize_t>(height()));
      if (static_cast<ssize_t>(m_SelectedLine)-static_cast<ssize_t>(height()) >= 0)
        m_SelectedLine -= height();
      else
        m_SelectedLine = 0;
    }
    else if (c == '\n' || c == '\r')
    {
      if(swapThread(state, pScreen))
      {
        bStop = true;
        bReturn = false;
      }
    }
    else if (c == 'q')
      bStop = true;
  }

  // HACK:: Serial connections will fill the screen with the last background colour used.
  //        Here we write a space with black background so the CLI screen doesn't get filled
  //        by some random colour!
  pScreen->drawString(" ", 1, 0, DebuggerIO::White, DebuggerIO::Black);
  pScreen->enableCli();
  return bReturn;
}

const char *ThreadsCommand::getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  static NormalStaticString Line;
  Line.clear();

  // Work through our process list.
  size_t idx = 0;
  Process *tehProcess = 0;
  Thread *tehThread = 0;
  bool stop = false;
  for (size_t i = 0; i < Scheduler::instance().getNumProcesses(); i++)
  {
    tehProcess = Scheduler::instance().getProcess(i);
    if (index == idx)
    {
      tehThread = 0;
      break;
    }
    idx++;
    
    for (size_t j = 0; j < tehProcess->getNumThreads(); j++)
    {
      if (index == idx)
      {
        tehThread = tehProcess->getThread(j);
        stop = true;
        break;
      }
      idx++;
    }
    if (stop)
      break;
  }

  // If this is just a process line.
  colour = DebuggerIO::Yellow;
  if (index == m_SelectedLine)
    bgColour = DebuggerIO::Blue;
  else
    bgColour = DebuggerIO::Black;
  if (tehThread == 0)
  {
    Line += "[";
    Line += tehProcess->getId();
    Line += "] ";
    Line += tehProcess->description();
  }
  else
  {
    Line += " | ";
  }
  
  return Line;
}
const char *ThreadsCommand::getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  static LargeStaticString Line;
  Line.clear();

  // Work through our process list.
  size_t idx = 0;
  Process *tehProcess = 0;
  Thread *tehThread = 0;
  bool stop = false;
  for (size_t i = 0; i < Scheduler::instance().getNumProcesses(); i++)
  {
    tehProcess = Scheduler::instance().getProcess(i);
    if (index == idx)
    {
      tehThread = 0;
      break;
    }
    idx++;
    for (size_t j = 0; j < tehProcess->getNumThreads(); j++)
    {
      if (index == idx)
      {
        tehThread = tehProcess->getThread(j);
        stop = true;
        break;
      }
      idx++;
    }
    if (stop)
      break;
  }

  if (tehThread != 0 && tehThread != Processor::information().getCurrentThread())
  {
    Line += "[";
    Line += tehThread->getId();
    Line += "] @ ";
    uintptr_t ip;
    if (tehThread->getInterruptState())
      ip = tehThread->getInterruptState()->getInstructionPointer();
    else
      ip = tehThread->state().getInstructionPointer();
  
    uintptr_t symStart;
    const char *pSym = KernelElf::instance().lookupSymbol(ip, &symStart);
    LargeStaticString sym;
    demangle_full(LargeStaticString(pSym), sym);
    Line.append(ip, 16);
    Line += ": ";
    Line += sym;
    colour = DebuggerIO::LightGrey;
  }
  else if (tehThread != 0) // tehThread == g_pCurrentThread
  {
    Line += "[";
    Line += tehThread->getId();
    Line += "] - CURRENT";
    colour = DebuggerIO::Yellow;
  }
  
  if (index == m_SelectedLine)
    bgColour = DebuggerIO::Blue;
  else
    bgColour = DebuggerIO::Black;
  colOffset = 3;
  return Line;
}

size_t ThreadsCommand::getLineCount()
{
  return m_nLines;
}

bool ThreadsCommand::swapThread(InterruptState &state, DebuggerIO *pScreen)
{
  // Work through our process list.
  size_t idx = 0;
  Process *tehProcess = 0;
  Thread *tehThread = 0;
  bool stop = false;
  for (size_t i = 0; i < Scheduler::instance().getNumProcesses(); i++)
  {
    tehProcess = Scheduler::instance().getProcess(i);
    if (m_SelectedLine == idx)
    {
      tehThread = 0;
      break;
    }
    idx++;
    for (size_t j = 0; j < tehProcess->getNumThreads(); j++)
    {
      if (m_SelectedLine == idx)
      {
        tehThread = tehProcess->getThread(j);
        stop = true;
        break;
      }
      idx++;
    }
    if (stop)
      break;
  }

  // We can only swap to threads, not entire processes!
  if (tehThread == 0)
    return false;

  pScreen->destroy();
  Scheduler::instance().switchToAndDebug(state, tehThread);

  return true;
}

#endif
