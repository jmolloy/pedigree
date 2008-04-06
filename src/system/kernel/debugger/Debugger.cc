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

#include <Log.h>
#include <Debugger.h>
#include <DebuggerIO.h>
#include <LocalIO.h>
#include <SerialIO.h>
#include <DisassembleCommand.h>
#include <QuitCommand.h>
#include <PanicCommand.h>
#include <BreakpointCommand.h>
#include <DumpCommand.h>
#include <LogViewer.h>
#include <Backtracer.h>
#include <utilities/utility.h>
#include <StepCommand.h>
#include <TraceCommand.h>
#include <MemoryInspector.h>
#include <machine/Machine.h>

Debugger Debugger::m_Instance;

TraceCommand g_Trace;

/// Helper function. Returns the index of a command in pCommands that matches prefix. Starts searching
/// through pCommands at index start. Returns -1 if none found.
static int getCommandMatchingPrefix(char *prefix, DebuggerCommand **pCommands, size_t nCmds, size_t start)
{
  for (size_t i = start; i < nCmds; i++)
  {
    if (!strncmp(pCommands[i]->getString(), prefix, strlen(prefix)))
      return i;
  }
  return -1;
}

/// Helper function. Returns true if the string in pStr matches pCommand. If so, pStr is changed
/// so that on return, it contains the parameters to that command (everything after the first space).
static bool matchesCommand(char *pStr, DebuggerCommand *pCommand)
{
  if (!strncmp(pCommand->getString(), pStr, strlen(pCommand->getString())))
  {
    size_t n = strlen(pCommand->getString());
    memcpy(pStr, pStr+n+1, strlen(pStr)-n);
    return true;
  }
  else
  {
    return false;
  }
}

Debugger::Debugger() :
  m_nIoType(DEBUGGER)
{

}

Debugger::~Debugger()
{
}

void Debugger::initialise()
{
  if (!InterruptManager::instance().registerInterruptHandlerDebugger(InterruptManager::instance().getBreakpointInterruptNumber(), this))
    ERROR("Debugger: breakpoint interrupt registration failed!");
  if (!InterruptManager::instance().registerInterruptHandlerDebugger(InterruptManager::instance().getDebugInterruptNumber(), this))
    ERROR("Debugger: debug interrupt registration failed!");
}

void Debugger::breakpoint(InterruptState &state)
{
  /*
   * I/O implementations.
   */
  LocalIO localIO(Machine::instance().getVga(0), Machine::instance().getKeyboard());
  SerialIO serialIO(Machine::instance().getSerial(0));
  SerialIO serialIO2(Machine::instance().getSerial(1));
  
 DebuggerIO *pInterfaces[] = {&localIO, &serialIO, &serialIO2};
 int nInterfaces = 3;
//   DebuggerIO *pInterfaces[] = {&localIO};
//   int nInterfaces=1;
  // IO interface.
  DebuggerIO *pIo = 0;
  int nChosenInterface = -1;
  
  // Commands.
  DisassembleCommand disassembler;
  LogViewer logViewer;
  Backtracer backtracer;
  QuitCommand quit;
  BreakpointCommand breakpoint;
  DumpCommand dump;
  StepCommand step;
  MemoryInspector memory;
  PanicCommand panic;
  
  size_t nCommands = 10;
  DebuggerCommand *pCommands[] = {&disassembler,
                                  &logViewer,
                                  &backtracer,
                                  &quit,
                                  &breakpoint,
                                  &dump,
                                  &step,
                                  &memory,
                                  &g_Trace,
                                  &panic};
  

  // Are we going to jump directly into the tracer? In which case bypass device detection.
  int n = g_Trace.execTrace();
  if (n == -1)
  {
    // Write a "Press any key..." message to each device, then poll each device.
    // The first one with data waiting becomes the active device, all others are locked out.
    for (int i = 0; i < nInterfaces; i++)
    {
      pInterfaces[i]->disableCli();
      pInterfaces[i]->drawString("Press any key to enter the debugger...", 0, 0, DebuggerIO::LightBlue, DebuggerIO::Black);
    }
    // Poll each device.
    while (pIo == 0)
      for (int i = 0; i < nInterfaces; i++)
      {
        char c = pInterfaces[i]->getCharNonBlock();
        if ((c >= 32 && c <= 127) || c == '\n' || c == 0x08 || c == '\r' || c == 0x09)
        {
          pIo = pInterfaces[i];
          nChosenInterface = i;
          break;
        }
      }
  }
  else
  {
    pIo = pInterfaces[n];
    nChosenInterface = n;
  }
  pIo->readDimensions();

  // Say sorry to the losers...
  for (int i = 0; i < nInterfaces; i++)
    if (pIo != pInterfaces[i])
      pInterfaces[i]->drawString("Locked by another device.", 1, 0, DebuggerIO::LightRed, DebuggerIO::Black);
  
  pIo->setCliUpperLimit(1); // Give us room for a status bar on top.
  pIo->setCliLowerLimit(1); // And a status bar on the bottom.
  pIo->enableCli(); // Start CLI mode.
  
  // Main CLI loop.
  bool bKeepGoing = false;
  do
  {
    HugeStaticString command;
    HugeStaticString output;
    // Should we jump directly in to the tracer?
    if (g_Trace.execTrace() != -1)
    {
      bKeepGoing = g_Trace.execute(command, output, state, pIo);
      continue;
    }
    else
      g_Trace.setInterface(nChosenInterface);
    // Clear the top and bottom status lines.
    pIo->drawHorizontalLine(' ', 0, 0, pIo->getWidth()-1, DebuggerIO::White, DebuggerIO::Green);
    pIo->drawHorizontalLine(' ', pIo->getHeight()-1, 0, pIo->getWidth()-1, DebuggerIO::White, DebuggerIO::Green);
    // Write the correct text in the upper status line.
    pIo->drawString("Pedigree debugger", 0, 0, DebuggerIO::White, DebuggerIO::Green);
  
    bool matchedCommand = false;
    DebuggerCommand *pAutoComplete = 0;
    while(1)
    {
      // Try and get a character from the CLI, passing in a buffer to populate and an
      // autocomplete command for if the user presses TAB (if one is defined).
      if (pIo->readCli(command, pAutoComplete))
        break; // Command complete, try and parse it.
  
      // The command wasn't complete - let's parse it and try and get an autocomplete string.
      HugeStaticString str;
      NormalStaticString str2;
      matchedCommand = false;
      for (size_t i = 0; i < nCommands; i++)
      {
        // TODO: This cast is completly wrong. As I said, don't touch (as in 'write') StaticString's
        //       internal string. The const is not there because I don't like you :-), it is there
        //       because directly writing is not garantueed to work (and it actually will break our code).
        if (matchesCommand(const_cast<char*>(static_cast<const char*>(command)), pCommands[i]))
        {
          str2 = static_cast<const char *>(pCommands[i]->getString());
          str2 += ' ';
          pCommands[i]->autocomplete(command, str);
          matchedCommand = true;
          break;
        }
      }

      pAutoComplete = 0;
      if (!matchedCommand)
      {
        int i = -1;
        while ( (i = getCommandMatchingPrefix(const_cast<char*>(static_cast<const char*>(command)), pCommands, nCommands, i+1)) != -1)
        {
          if (!pAutoComplete)
            pAutoComplete = pCommands[i];
          str += static_cast<const char*> (pCommands[i]->getString());
          str += " ";
        }
      }
      
      pIo->drawHorizontalLine(' ', pIo->getHeight()-1, 0, pIo->getWidth()-1, DebuggerIO::White, DebuggerIO::Green);
      pIo->drawString(str2, pIo->getHeight()-1, 0, DebuggerIO::Yellow, DebuggerIO::Green);
      pIo->drawString(str, pIo->getHeight()-1, str2.length(), DebuggerIO::White, DebuggerIO::Green);
    }
  
    // A command was entered.
    bool bValidCommand = false;
    for (size_t i = 0; i < nCommands; i++)
    {
      if (matchesCommand(const_cast<char*>(static_cast<const char*>(command)), pCommands[i]))
      {
        bKeepGoing = pCommands[i]->execute(command, output, state, pIo);
        pIo->writeCli(output, DebuggerIO::LightGrey, DebuggerIO::Black);
        bValidCommand = true;
      }
    }
    
    if (!bValidCommand)
    {
      pIo->writeCli("Unrecognised command.\n", DebuggerIO::LightGrey, DebuggerIO::Black);
      bKeepGoing = true;
    }
  
  }
  while (bKeepGoing);
}

void Debugger::interrupt(size_t interruptNumber, InterruptState &state)
{
  // We switch here on the interrupt number, and dispatch accordingly.
  if (interruptNumber == InterruptManager::instance().getBreakpointInterruptNumber())
  {
    breakpoint(state);
  }
  else if (interruptNumber == InterruptManager::instance().getDebugInterruptNumber())
  {
    Processor::setSingleStep(false, state);
    breakpoint(state);
  }
}
