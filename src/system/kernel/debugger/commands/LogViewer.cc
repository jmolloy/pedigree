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
#include "LogViewer.h"
#include <Log.h>
#include <utilities/utility.h>
#include <DebuggerIO.h>

LogViewer::LogViewer()
 : DebuggerCommand()
{
}

LogViewer::~LogViewer()
{
}

void LogViewer::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool LogViewer::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
  // Let's enter 'raw' screen mode.
  pScreen->disableCli();
  
  // Clear the top and bottom status lines.
  pScreen->drawHorizontalLine(' ', 0, 0, pScreen->getWidth()-1,
                          DebuggerIO::White, DebuggerIO::Green);
  pScreen->drawHorizontalLine(' ', pScreen->getHeight()-1, 0, pScreen->getWidth()-1,
                              DebuggerIO::White, DebuggerIO::Green);
  // Write the correct text in the upper status line.
  pScreen->drawString("Pedigree debugger - Log Viewer", 0, 0, DebuggerIO::White, DebuggerIO::Green);
  // Write some helper text in the lower status line.
  pScreen->drawString("j: Up one line. k: Down one line. backspace: Page up. space: Page down. q: Quit", 
                      pScreen->getHeight()-1, 0, DebuggerIO::White, DebuggerIO::Green);
  pScreen->drawString("j", pScreen->getHeight()-1, 0, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("k", pScreen->getHeight()-1, 16, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("backspace", pScreen->getHeight()-1, 34, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("space", pScreen->getHeight()-1, 54, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("q", pScreen->getHeight()-1, 72, DebuggerIO::Yellow, DebuggerIO::Green);
  
  // How many usable lines do we have?
  size_t nLines = pScreen->getHeight()-2; // -2 for top and bottom status bars.
  
  // Starting line.
  size_t line = Log::instance().getEntryCount() - nLines;

  // Main loop.
  bool bStop = false;
  while(!bStop)
  {
    refresh(line, pScreen);
    // Wait for input.
    char c;

    while( (c=pScreen->getChar()) == 0)
      ;
    switch(c)
    {
    case 'j':
    {
      if (line > 0)
        line--;
      break;
    }
    case 'k':
    {
      if (line < Log::instance().getEntryCount() - nLines)
        line++;
      break;
    }
    case ' ':
    {
      if (Log::instance().getEntryCount() <= nLines)
        break;
      if (line+nLines <= Log::instance().getEntryCount()-nLines)
        line += nLines;
      else
        line = Log::instance().getEntryCount()-nLines;
      break;
    }
    case 0x08:
    {
      if (Log::instance().getEntryCount() <= nLines)
        break;
      if (line-nLines >= 0)
        line -= nLines;
      else
        line = 0;
      break;
    }
    case 'q':
      bStop = true;
    }
  }
  
  pScreen->enableCli();

  return true;
}

void LogViewer::refresh(size_t topLine, DebuggerIO *pScreen)
{
  size_t line = topLine;
  pScreen->disableRefreshes();
  
  // For every available line.
  for (size_t i = 1 /* Top status line */; i < pScreen->getHeight()-1; i++)
  {
    pScreen->drawHorizontalLine(' ', i, 0, pScreen->getWidth()-1,
                               DebuggerIO::White, DebuggerIO::Black);
    if (line >= 0)
    {
      Log::LogEntry_t entry = Log::instance().getEntry(line);
      
      char timestamp[16];
      sprintf(timestamp, "[%08d]", entry.timestamp);
      
      DebuggerIO::Colour colour = DebuggerIO::White;
      switch (entry.type)
      {
      case Log::Notice:
        colour = DebuggerIO::Green;
        break;
      case Log::Warning:
        colour = DebuggerIO::Yellow;
        break;
      case Log::Error:
        colour = DebuggerIO::Magenta;
        break;
      case Log::Fatal:
        colour = DebuggerIO::Red;
        break;
      }
      
      pScreen->drawString( timestamp, i, 0, colour, DebuggerIO::Black);
      pScreen->drawString( entry.str, i, 11, DebuggerIO::LightGrey, DebuggerIO::Black );
    }
    line++;
  }
  
  pScreen->enableRefreshes();
}
