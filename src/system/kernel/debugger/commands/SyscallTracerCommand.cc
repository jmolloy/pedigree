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
#include "SyscallTracerCommand.h"

SyscallTracerCommand::SyscallTracerCommand()
{

}

SyscallTracerCommand::~SyscallTracerCommand()
{

}

void SyscallTracerCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{

}

bool SyscallTracerCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
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
  pScreen->drawString("Pedigree debugger - System Call Tracer",
                     0,
                     0,
                     DebuggerIO::White,
                     DebuggerIO::Green);

  // Main loop.
  bool bStop = false;
  while(!bStop)
  {
    refresh(pScreen);

    // Wait for input.
    char c = 0;
    while( !(c=pScreen->getChar()) )
      ;

    if (c == 'q')
          bStop = true;
  }

  //  Return to the debugger
  return(true);
}

const char *SyscallTracerCommand::getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  return(0);
}

const char *SyscallTracerCommand::getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  return(0);
}

size_t SyscallTracerCommand::getLineCount()
{
  return(0);
}
