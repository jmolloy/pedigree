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
#include <DisassembleCommand.h>
#include <utility.h>

Debugger::Debugger()
{
}

Debugger::~Debugger()
{
}

int getCommandMatchingPrefix(char *prefix, DebuggerCommand **pCommands, int nCmds, int start)
{
  for (int i = start; i < nCmds; i++)
  {
    if (!strncmp(pCommands[i]->getString(), prefix, strlen(prefix)))
      return i;
  }
  return -1;
}

bool matchesCommand(char *pStr, DebuggerCommand *pCommand)
{
  if (!strncmp(pCommand->getString(), pStr, strlen(pCommand->getString())))
  {
    int n = strlen(pCommand->getString());
    memcpy((unsigned char*)pStr, (unsigned char*)pStr+n, strlen(pStr)-n+1);
    return true;
  }
  else
  {
    return false;
  }
}

void Debugger::breakpoint(int type)
{

  DebuggerIO *pIo;
  LocalIO localIO;
  DisassembleCommand disassembler;
  //SerialIO serialIO;
  
  int nCommands = 1;
  DebuggerCommand *pCommands[] = {&disassembler};
  
  if (type == MONITOR)
  {
    pIo = &localIO;
  }
  else
  {
    // serial
  }
  
  pIo->setCliUpperLimit(1); // Give us room for a status bar on top.
  pIo->setCliLowerLimit(1); // And a status bar on the bottom.
  pIo->enableCli();

  pIo->drawHorizontalLine(' ', 0, 0, 79, DebuggerIO::White, DebuggerIO::DarkGrey);
  pIo->drawHorizontalLine(' ', 24, 0, 79, DebuggerIO::White, DebuggerIO::DarkGrey);
  pIo->drawString("Pedigree debugger", 0, 0, DebuggerIO::White, DebuggerIO::DarkGrey);

  char pCommand[256];
  while(1)
  {
    if (pIo->readCli(pCommand, 256))
      break;

    char pStr[256];
    char pStr2[64];
    bool matchedCommand = false;
    for (int i = 0; i < nCommands; i++)
    {
      if (matchesCommand(pCommand, pCommands[i]))
      {
        strcpy(pStr2, pCommands[i]->getString());
        strcat(pStr2, " ");
        pCommands[i]->autocomplete(pCommand, pStr);
        matchedCommand = true;
        break;
      }
    }

    if (!matchedCommand)
    {
      pStr2[0] = '\0';
      pStr[0] = '\0';
      int i = -1;
      while ( (i = getCommandMatchingPrefix(pCommand, pCommands, nCommands, i+1)) != -1)
      {
        strcat (pStr, pCommands[i]->getString());
        strcat (pStr, " ");
      }
    }
    
    pIo->drawHorizontalLine(' ', 24, 0, 79, DebuggerIO::White, DebuggerIO::DarkGrey);
    pIo->drawString(pStr2, 24, 0, DebuggerIO::Yellow, DebuggerIO::DarkGrey);
    pIo->drawString(pStr, 24, strlen(pStr2), DebuggerIO::White, DebuggerIO::DarkGrey);
  }

  for(;;);

}

