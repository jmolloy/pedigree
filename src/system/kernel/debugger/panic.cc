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

#include <Debugger.h>

#include <DebuggerIO.h>
#include <LocalIO.h>
#include <SerialIO.h>

#include <utilities/StaticString.h>

#include <processor/Processor.h>
#include <machine/Machine.h>

#include <Log.h>
#include <utilities/utility.h>

void _panic( const char* msg, DebuggerIO* pScreen )
{  
  HugeStaticString panic_output;
  panic_output.clear();
  
  panic_output.append( "PANIC: " );
  panic_output.append( msg );
  //pScreen->drawString( "PANIC: ", 0, 0, DebuggerIO::Black, DebuggerIO::White );
  //pScreen->drawString( msg, 0, 7, DebuggerIO::Black, DebuggerIO::White );

  Log &log = Log::instance();
  Log::SeverityLevel level;
  static NormalStaticString Line;

  size_t iEntry, iUsedEntries = 0;
  bool bPrintThisLine = false;
  for( iEntry = 0; iUsedEntries < 4 && iEntry < (log.getStaticEntryCount() + log.getDynamicEntryCount()); iEntry++ )
  {
    if( iEntry < log.getStaticEntryCount() )
    {
      const Log::StaticLogEntry &entry = log.getStaticEntry(iEntry);
      level = entry.type;
      
      if( level == Log::Fatal || level == Log::Error )
      {
        Line.clear();
        Line.append("[");
        Line.append(entry.timestamp, 10, 8, '0');
        Line.append("] ");
        Line.append(entry.str);
        Line.append( "\n" );
        
        iUsedEntries++;
        
        bPrintThisLine = true;
      }
    }
    else
    {
      const Log::DynamicLogEntry &entry = log.getDynamicEntry(iEntry);
      level = entry.type;
      
      if( level == Log::Fatal || level == Log::Error )
      {
        Line.clear();
        Line.append("[");
        Line.append(entry.timestamp, 10, 8, '0');
        Line.append("] ");
        Line.append(entry.str);
        Line.append( "\n" );
        
        iUsedEntries++;
        
        bPrintThisLine = true;
      }
    }

    // print the line
    if( bPrintThisLine = true )
    {
      panic_output.append( Line );
      //pScreen->drawString( Line, iUsedEntries, 0, DebuggerIO::Black, DebuggerIO::White );
      bPrintThisLine = false;
    }
  }
  
  // write the final string to the screen
  pScreen->drawString( panic_output, 0, 0, DebuggerIO::Black, DebuggerIO::White );
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
