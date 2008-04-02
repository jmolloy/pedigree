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
#include <utilities/utility.h>
#include <DebuggerIO.h>
#include <processor/Processor.h>
#include <udis86.h>
#include <FileLoader.h>
#include <Log.h>
#include <demangle.h>
#include <machine/Machine.h>

// TEMP!
extern FileLoader *g_pKernel;

static void addToBuffer(uintptr_t n, uintptr_t *pBuffer, unsigned int &nInstr, unsigned int nLinesToCache)
{
  if (nInstr == nLinesToCache)
  {
    for (unsigned int i = 1; i < nLinesToCache; i++)
      pBuffer[i-1] = pBuffer[i];
    pBuffer[nLinesToCache-1] = n;
  }
  else
    pBuffer[nInstr++] = n;
}

TraceCommand::TraceCommand()
  : DebuggerCommand(),
    m_nExec(-1),
    m_nInterface(-1)
{
}

TraceCommand::~TraceCommand()
{
}

void TraceCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

void TraceCommand::setInterface(int nInterface)
{
  m_nInterface = nInterface;
}

bool TraceCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
  m_nExec = -1;
  // Let's enter 'raw' screen mode.
  pScreen->disableCli();
  
  // Work out where the split will be between the disassembler, reg dump and stacktrace.
  // Give the reg dump 25 columns, hardcoded.
  int nCols = pScreen->getWidth()-25;
  // Give the disassembler 3/4 of the available lines.
  int nLines = ((pScreen->getHeight()-2) * 3) / 4;
  
  Disassembly disassembly(state);
  disassembly.move(0, 1);
  disassembly.resize(nCols, nLines);
  
  Registers registers(state);
  registers.move(nCols+1, 1);
  registers.resize(24, nLines);
  
  Stacktrace stacktrace(state);
  stacktrace.move(0, nLines+2);
  stacktrace.resize(pScreen->getWidth(), pScreen->getHeight()-nLines-3);
  
  // Here we enter our main runloop.
  bool bContinue = true;
  while (bContinue)
  {
    pScreen->disableRefreshes();
    drawBackground(nCols, nLines, pScreen);
    registers.refresh(pScreen);
    disassembly.refresh(pScreen);
    stacktrace.refresh(pScreen);
    pScreen->enableRefreshes();
    
    char c;
    while( (c=pScreen->getChar()) == 0)
      ;
    if (c == 'q')
      break;
    if (c == 's')
    {
      m_nExec = m_nInterface;
      // HACK on real boxen the screen mode change takes quite a while. If we're single stepping
      // don't bother changing mode back.
      Machine::instance().getVga(0)->rememberMode();
      Processor::setSingleStep(true, state);
      return false;
    }
    
  }
  
  // Let's enter CLI screen mode again.
  pScreen->enableCli();
  return true; // Return control to the debugger.
}

void TraceCommand::drawBackground(size_t nCols, size_t nLines, DebuggerIO *pScreen)
{
  // Destroy all text.
  pScreen->cls();

  // Clear the top and bottom status lines.
  pScreen->drawHorizontalLine(' ', 0, 0, pScreen->getWidth()-1,
                              DebuggerIO::White, DebuggerIO::Green);
  pScreen->drawHorizontalLine(' ', pScreen->getHeight()-1, 0, pScreen->getWidth()-1,
                              DebuggerIO::White, DebuggerIO::Green);
  // Write the correct text in the upper status line.
  pScreen->drawString("Pedigree debugger - Execution Tracer", 0, 0, DebuggerIO::White, DebuggerIO::Green);
  // Write some helper text in the lower status line.
  pScreen->drawString("s: Step. c: Continue. q: Quit. ?: Current instruction. ?: Breakpoint.", 
                      pScreen->getHeight()-1, 0, DebuggerIO::White, DebuggerIO::Green);
  pScreen->drawString("s", pScreen->getHeight()-1, 0, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("c", pScreen->getHeight()-1, 9, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("q", pScreen->getHeight()-1, 22, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString(" ", pScreen->getHeight()-1, 31, DebuggerIO::Yellow, DebuggerIO::Blue);
  pScreen->drawString(" ", pScreen->getHeight()-1, 55, DebuggerIO::Yellow, DebuggerIO::Red);

  pScreen->drawHorizontalLine('-', nLines+1, 0, pScreen->getWidth(), DebuggerIO::DarkGrey, DebuggerIO::Black);
  pScreen->drawVerticalLine('|', nCols, 1, nLines, DebuggerIO::DarkGrey, DebuggerIO::Black);
}

TraceCommand::Disassembly::Disassembly(InterruptState &state)
  : m_nInstructions(0), m_nFirstInstruction(0), m_nIp(0)
{
  // Try and count how many lines of instructions we have.
  m_nIp = state.getInstructionPointer();
  uintptr_t nSymStart = 0;
  g_pKernel->lookupSymbol(m_nIp, &nSymStart);
  
  // TODO use disassembler abstraction.
  ud_t ud_obj;
  ud_init(&ud_obj);
#ifdef X86
  ud_set_mode(&ud_obj, 32);
#endif
#ifdef X64
  ud_set_mode(&ud_obj, 64);
#endif
  ud_set_syntax(&ud_obj, UD_SYN_INTEL);
  ud_set_pc(&ud_obj, nSymStart);
  ud_set_input_buffer(&ud_obj, reinterpret_cast<uint8_t*>(nSymStart), 4096);
  
  m_nFirstInstruction = nSymStart;
  uintptr_t nLocation = 0;
  while (true)
  {
    ud_disassemble(&ud_obj);
    nLocation = ud_insn_off(&ud_obj);
    uintptr_t nSym;
    g_pKernel->lookupSymbol(nLocation, &nSym);
    if (nSym == nLocation && nSym != nSymStart) // New symbol. Quit.
      break;
    m_nInstructions++;
  }
}

const char *TraceCommand::Disassembly::getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  static LargeStaticString sym;
  if (index == 0)
  {
    // We treat index == 0 slightly differently - it's the symbol name.
    colour = DebuggerIO::Yellow;
    uintptr_t nSym;
    const char *pSym = g_pKernel->lookupSymbol(m_nFirstInstruction, &nSym);
    sym.clear();
    demangle_full(LargeStaticString(pSym), sym);
    sym += ":";
    return sym;
  }
  index --; // Get rid of index 0.

  size_t nInstruction = 0;
   // TODO use disassembler abstraction.
  ud_t ud_obj;
  ud_init(&ud_obj);
#ifdef X86
  ud_set_mode(&ud_obj, 32);
#endif
#ifdef X64
  ud_set_mode(&ud_obj, 64);
#endif
  ud_set_syntax(&ud_obj, UD_SYN_INTEL);
  ud_set_pc(&ud_obj, m_nFirstInstruction);
  ud_set_input_buffer(&ud_obj, reinterpret_cast<uint8_t*>(m_nFirstInstruction), 4096);
  
  uintptr_t nLocation;
  while (nInstruction <= index)
  {
    ud_disassemble(&ud_obj);
    nInstruction++;
  }
  nLocation = ud_insn_off(&ud_obj);

  // If this is our actual instruction location, put a blue background over it.
  if (nLocation == m_nIp)
    bgColour = DebuggerIO::Blue;
  
  // We want grey text.
  colour = DebuggerIO::DarkGrey;
  
  // The text being...
  sym.clear();
  sym.append(nLocation, 16, sizeof(uintptr_t)*2, '0');
  sym += ' ';
  return sym;
}

const char *TraceCommand::Disassembly::getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  static LargeStaticString sym;
  if (index == 0)
  {
    // We treat index == 0 slightly differently - it's the symbol name.
    return 0;
  }
  index --; // Get rid of index 0.

  size_t nInstruction = 0;
   // TODO use disassembler abstraction.
  ud_t ud_obj;
  ud_init(&ud_obj);
#ifdef X86
  ud_set_mode(&ud_obj, 32);
#endif
#ifdef X64
  ud_set_mode(&ud_obj, 64);
#endif
  ud_set_syntax(&ud_obj, UD_SYN_INTEL);
  ud_set_pc(&ud_obj, m_nFirstInstruction);
  ud_set_input_buffer(&ud_obj, reinterpret_cast<uint8_t*>(m_nFirstInstruction), 4096);
  
  uintptr_t nLocation;
  while (nInstruction <= index)
  {
    ud_disassemble(&ud_obj);
    nInstruction++;
  }
  nLocation = ud_insn_off(&ud_obj);
  
  // If this is our actual instruction location, put a blue background over it.
  if (nLocation == m_nIp)
    bgColour = DebuggerIO::Blue;
  
  // We want white text.
  colour = DebuggerIO::White;
  
  // At offset...
  colOffset = sizeof(uintptr_t)*2+1;
  
  // The text being...
  sym.clear();
  sym += ud_insn_asm(&ud_obj);
  uint32_t nLen = sym.length() + colOffset;
  while(nLen++ < width())
    sym += ' ';
  return sym;
}

size_t TraceCommand::Disassembly::getLineCount()
{
  return m_nInstructions+1;
}


TraceCommand::Registers::Registers(InterruptState &state)
  : m_State(state)
{
}

const char *TraceCommand::Registers::getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  static LargeStaticString str;
  colour = DebuggerIO::Yellow;
  // We treat index 0 slightly differently.
  if (index == 0)
  {
    str.clear();
    if (m_State.kernelMode())
      str = "Kernel mode";
    else
      str = "User mode";
    return str;
  }
  else
  {
    int nRegister = index-1;
    str.clear();
    str = m_State.getRegisterName(nRegister);
    return str;
  }
}

const char *TraceCommand::Registers::getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  static LargeStaticString str;
  // We treat index 0 slightly differently.
  if (index == 0)
  {
    return 0;
  }
  else
  {
    int nRegister = index-1;
    str.clear();
    uintptr_t nValue = m_State.getRegister(nRegister);
    str = "0x";
    str.append(nValue, 16, sizeof(uintptr_t)*2, '0');
    colOffset = width()- 2*sizeof(uintptr_t) - 2;
    return str;
  }
}

size_t TraceCommand::Registers::getLineCount()
{
  return m_State.getRegisterCount()+1;
}

TraceCommand::Stacktrace::Stacktrace(InterruptState &state)
  : m_Bt()
{
  m_Bt.performBacktrace(state);
}

const char *TraceCommand::Stacktrace::getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  static HugeStaticString str;
  str.clear();
  m_Bt.prettyPrint(str, 1, index);
  str = str.left(str.length()-1);
  return str;
}

const char *TraceCommand::Stacktrace::getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  return 0;
}

size_t TraceCommand::Stacktrace::getLineCount()
{
  return m_Bt.numStackFrames();
}
