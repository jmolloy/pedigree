/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#include "TraceCommand.h"
#include <Log.h>
#include <utilities/utility.h>
#include <DebuggerIO.h>
#include <processor/processor.h>

TraceCommand::TraceCommand()
  : DebuggerCommand(),
    m_bExec(false)
{
}

TraceCommand::~TraceCommand()
{
}

void TraceCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool TraceCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
  // Let's enter 'raw' screen mode.
  pScreen->disableCli();
  
  // Work out where the split will be between the disassembler, reg dump and stacktrace.
  // Give the reg dump 25 columns, hardcoded.
  int nCols = 55;
  // Give the disassembler 3/4 of the available lines.
  int nLines = ((pScreen->getHeight()-2) * 3) / 4;
  
  // Here we enter our main runloop.
  bool bContinue = true;
  while (bContinue)
  {
    pScreen->disableRefreshes();
    drawBackground(nCols, nLines, pScreen);
    drawRegisters(nCols, nLines, pScreen);
    drawDisassembly(nLines, pScreen);
    drawStacktrace((pScreen->getHeight()-2)-nLines, pScreen);
    pScreen->enableRefreshes();
    
    char c;
    while( (c=pScreen->getChar()) == 0)
      ;
    if (c == 'q')
      break;
    
  }
  
  // Let's enter CLI screen mode again.
  pScreen->enableCli();
  return true; // Return control to the debugger.
}

void TraceCommand::drawBackground(int nCols, int nLines, DebuggerIO *pScreen)
{
  // Destroy all text.
  for (int i = 0; i < pScreen->getHeight(); i++)
    pScreen->drawHorizontalLine(' ', i, 0, pScreen->getWidth()-1, DebuggerIO::White, DebuggerIO::Black);

  // Clear the top and bottom status lines.
  pScreen->drawHorizontalLine(' ', 0, 0, pScreen->getWidth()-1,
                              DebuggerIO::White, DebuggerIO::DarkGrey);
  pScreen->drawHorizontalLine(' ', pScreen->getHeight()-1, 0, pScreen->getWidth()-1,
                              DebuggerIO::White, DebuggerIO::DarkGrey);
  // Write the correct text in the upper status line.
  pScreen->drawString("Pedigree debugger - Execution Tracer", 0, 0, DebuggerIO::White, DebuggerIO::DarkGrey);
  // Write some helper text in the lower status line.
  pScreen->drawString("j: Up one line. k: Down one line. backspace: Page up. space: Page down. q: Quit", 
                      pScreen->getHeight()-1, 0, DebuggerIO::White, DebuggerIO::DarkGrey);
  pScreen->drawString("j", pScreen->getHeight()-1, 0, DebuggerIO::Yellow, DebuggerIO::DarkGrey);
  pScreen->drawString("k", pScreen->getHeight()-1, 16, DebuggerIO::Yellow, DebuggerIO::DarkGrey);
  pScreen->drawString("backspace", pScreen->getHeight()-1, 34, DebuggerIO::Yellow, DebuggerIO::DarkGrey);
  pScreen->drawString("space", pScreen->getHeight()-1, 54, DebuggerIO::Yellow, DebuggerIO::DarkGrey);
  pScreen->drawString("q", pScreen->getHeight()-1, 72, DebuggerIO::Yellow, DebuggerIO::DarkGrey);

  pScreen->drawHorizontalLine('-', nLines+1, 0, pScreen->getWidth(), DebuggerIO::DarkGrey, DebuggerIO::Black);
  pScreen->drawVerticalLine('|', nCols, 1, nLines, DebuggerIO::DarkGrey, DebuggerIO::Black);
}

void TraceCommand::drawDisassembly(int nLines, DebuggerIO *pScreen)
{
  
}

void TraceCommand::drawRegisters(int nCols, int nLines, DebuggerIO *pScreen)
{
}

void TraceCommand::drawStacktrace(int nLines, DebuggerIO *pScreen)
{
}