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



#include <Debugger.h>

#include <DebuggerIO.h>
#include <LocalIO.h>
#include <SerialIO.h>

#include <utilities/StaticString.h>

#include <processor/Processor.h>
#include <machine/Machine.h>

#include <Log.h>
#include <utilities/utility.h>

static size_t newlineCount(const char *pString)
{
  size_t nNewlines = 0;
  while (*pString != '\0')
    if (*pString++ == '\n')
      ++nNewlines;

  return nNewlines;
}

// TODO: We might want a separate parameter for a stacktrace/register dump
void _panic( const char* msg, DebuggerIO* pScreen )
{
  static HugeStaticString panic_output;
  panic_output.clear();

  panic_output.append( "PANIC: " );
  panic_output.append( msg );

  // write the final string to the screen
  pScreen->drawString( panic_output, 0, 0, DebuggerIO::Red, DebuggerIO::Black );

  size_t nLines = newlineCount(panic_output) + 2;

  Log &log = Log::instance();
  Log::SeverityLevel level;
  static NormalStaticString Line;

  size_t iEntry = 0, iUsedEntries = 0;
  if ((pScreen->getHeight() - nLines) < (log.getStaticEntryCount() + log.getDynamicEntryCount()))
    iEntry = log.getStaticEntryCount() + log.getDynamicEntryCount() - (pScreen->getHeight() - nLines) + 1;
  bool bPrintThisLine = false;
  for( ; iEntry < (log.getStaticEntryCount() + log.getDynamicEntryCount()); iEntry++ )
  {
    if( iEntry < log.getStaticEntryCount() )
    {
      const Log::StaticLogEntry &entry = log.getStaticEntry(iEntry);
      level = entry.type;

//      if( level == Log::Fatal || level == Log::Error )
//      {
        Line.clear();
        Line.append("[");
        Line.append(entry.timestamp, 10, 8, '0');
        Line.append("] ");
        Line.append(entry.str);
        Line.append( "\n" );

        bPrintThisLine = true;
//      }
    }
    else
    {
      const Log::DynamicLogEntry &entry = log.getDynamicEntry(iEntry);
      level = entry.type;

//      if( level == Log::Fatal || level == Log::Error )
//      {
        Line.clear();
        Line.append("[");
        Line.append(entry.timestamp, 10, 8, '0');
        Line.append("] ");
        Line.append(entry.str);
        Line.append( "\n" );

        bPrintThisLine = true;
//      }
    }

    // print the line
    if( bPrintThisLine == true )
    {
      ++iUsedEntries;
      pScreen->drawString( Line, nLines + iUsedEntries, 0, DebuggerIO::White, DebuggerIO::Black );
      bPrintThisLine = false;
    }
  }
}

void panic( const char* msg )
{
  /*
   * I/O implementations.
   */
  LocalIO localIO(Machine::instance().getVga(0), Machine::instance().getKeyboard());
  SerialIO serialIO(Machine::instance().getSerial(0));
  SerialIO serialIO2(Machine::instance().getSerial(1));

  DebuggerIO *pInterfaces[] = {&localIO, &serialIO, &serialIO2};
  int nInterfaces = 3;

  for( int nIFace = 0; nIFace < nInterfaces; nIFace++ )
    _panic( msg, pInterfaces[nIFace] );

  // Halt the processor
  Processor::halt();
}
