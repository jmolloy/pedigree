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
#include "BreakpointCommand.h"
#include <Log.h>
#include <utility.h>
#include <DebuggerIO.h>
#include <processor/processor.h>

BreakpointCommand::BreakpointCommand()
 : DebuggerCommand()
{
}

BreakpointCommand::~BreakpointCommand()
{
}

void BreakpointCommand::autocomplete(char *input, char *output, int len)
{
}

bool BreakpointCommand::execute(char *input, char *output, int len, InterruptState &state, DebuggerIO *pScreen)
{
  // Did we get any input?
  if (!strcmp(input, "breakpoint"))
  {
    // Print out the current breakpoint status.
    sprintf(output, "Current breakpoint status:\n");
    for(int i = 0; i < 4; i++)
    {
      char pStr[128];
      DebugFlags::FaultType nFt;
      DebugFlags::Length nLen;
      bool bEnabled;
      uintptr_t nAddress = Processor::getDebugBreakpoint(i, nFt, nLen, bEnabled);
      
      const char *pFaultType;
      switch (nFt)
      {
      case DebugFlags::InstructionFetch:
        pFaultType = "InstructionFetch";
        break;
      case DebugFlags::DataWrite:
        pFaultType = "DataWrite";
        break;
      case DebugFlags::IOReadWrite:
        pFaultType = "IOReadWrite";
        break;
      case DebugFlags::DataReadWrite:
        pFaultType = "DataReadWrite";
        break;
      }
      
      const char *pLength;
      switch (nLen)
      {
      case DebugFlags::OneByte:
        pLength = "OneByte";
        break;
      case DebugFlags::TwoBytes:
        pLength = "TwoBytes";
        break;
      case DebugFlags::FourBytes:
        pLength = "FourBytes";
        break;
      case DebugFlags::EightBytes:
        pLength = "EightBytes";
        break;
      }
      
      const char *pEnabled = "disabled";
      if (bEnabled) pEnabled = "enabled";
      
      
      sprintf(pStr, "%d: 0x%x \t%s \t%s \t%s\n", i, nAddress, pFaultType, pLength, pEnabled);
      strcat(output, pStr);
    }
  }

  return true;
}
