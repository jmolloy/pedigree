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
#include <Elf32.h>
#include <Log.h>
#include <demangle.h>
#include <Backtrace.h>

// TEMP!
extern Elf32 elf;

static void addToBuffer(unsigned int n, unsigned int *pBuffer, unsigned int &nInstr, unsigned int nLinesToCache)
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
  m_bExec = false;
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
    drawRegisters(nCols, nLines, pScreen, state);
    drawDisassembly(nCols, nLines, pScreen, state);
    drawStacktrace((pScreen->getHeight()-2)-nLines, pScreen, state);
    pScreen->enableRefreshes();
    
    char c;
    while( (c=pScreen->getChar()) == 0)
      ;
    if (c == 'q')
      break;
    if (c == 's')
    {
      m_bExec = true;
      Processor::setSingleStep(true, state);
      return false;
    }
    
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

void TraceCommand::drawDisassembly(int nCols, int nLines, DebuggerIO *pScreen, InterruptState &state)
{
  // We want the current instruction to be in the middle of the screen (ish).
  // The important thing is that to get the correct disassembly we must start disassembling
  // on a correct instruction boundary (and we can't go backwards), so we start at the start
  // of the current symbol.
  
  // Current symbol location.
  // TODO grep the memory map for the right ELF to look at.
  unsigned int ip = state.getInstructionPointer();
  unsigned int symStart = 0;
  const char *pSym = elf.lookupSymbol(ip, &symStart);

  ud_t ud_obj;
  ud_init(&ud_obj);
#ifdef X86
  ud_set_mode(&ud_obj, 32);
#endif
#ifdef X86_64
  ud_set_mode(&ud_obj, 64);
#endif
  ud_set_syntax(&ud_obj, UD_SYN_INTEL);
  ud_set_pc(&ud_obj, symStart);
  ud_set_input_buffer(&ud_obj, reinterpret_cast<uint8_t*>(symStart), 4096);
  
  // Let's just assume we'll never have more than 1024 lines total.
  unsigned int instrBuffer[512];
  unsigned int nInstr = 0;
  unsigned int nLinesToCache = nLines/2;

  unsigned int location = 0;
  int i = 0;
  while(location < ip)
  {
    ud_disassemble(&ud_obj);
    location = ud_insn_off(&ud_obj);
    unsigned int nSym;
    elf.lookupSymbol(location, &nSym);
    if (nSym == location) // New symbol. Add two lines.
    {
      addToBuffer(0, instrBuffer, nInstr, nLinesToCache);
      addToBuffer(0, instrBuffer, nInstr, nLinesToCache);
    }

    addToBuffer(location, instrBuffer, nInstr, nLinesToCache);
  }

  // OK, awesome, we have an instruction buffer. Let's disassemble it.
  ud_set_pc(&ud_obj, instrBuffer[0]);
  ud_set_input_buffer(&ud_obj, reinterpret_cast<uint8_t*>(instrBuffer[0]), 4096);
  unsigned int nLine;
  for (nLine = 0; nLine < static_cast<unsigned int>(nLines); nLine++)
  {
    ud_disassemble(&ud_obj);
    location = ud_insn_off(&ud_obj);
    unsigned int nSym;
    const char *pSym = elf.lookupSymbol(location, &nSym);
#ifdef BITS_32
    const int k_nAddrLen = 8;
#endif
#ifdef BITS_64
    const int k_nAddrLen = 16;
#endif
    NormalStaticString str;
    if (nSym <= location && nSym != symStart) // New symbol. Add two lines.
    {
      ud_set_pc(&ud_obj, nSym);
      ud_set_input_buffer(&ud_obj, reinterpret_cast<uint8_t*>(nSym), 4096);
      ud_disassemble(&ud_obj);
      location = ud_insn_off(&ud_obj);
      symStart = nSym;
      
      LargeStaticString sym;
      demangle_full(LargeStaticString(pSym), sym);
      
      nLine++; // Blank line.
      if (nLine >= static_cast<unsigned int>(nLines)) break;
      str = "<";
      str += sym.left(nCols-2);
      str += ">";
      pScreen->drawString(str, nLine+1, 0, DebuggerIO::Yellow, DebuggerIO::Black);
      nLine++;
      if (nLine >= static_cast<unsigned int>(nLines)) break;
    }
    
    // If this is our actual instruction location, put a blue background over it.
    DebuggerIO::Colour bg = DebuggerIO::Black;
    if (location == ip)
    {
      pScreen->drawHorizontalLine(' ', nLine+1, 0, nCols-1, DebuggerIO::White, DebuggerIO::Blue);
      bg = DebuggerIO::Blue;
    }
    
    str = "";
    str.append(location, 16, k_nAddrLen, ' ');
    pScreen->drawString(str, nLine+1, 0, DebuggerIO::DarkGrey, bg);
    str = ud_insn_asm(&ud_obj);
    pScreen->drawString(str, nLine+1, k_nAddrLen+1, DebuggerIO::White, bg);
  }
  
}

void TraceCommand::drawRegisters(int nCols, int nLines, DebuggerIO *pScreen, InterruptState &state)
{
  int nLine = 0;
  if (state.kernelMode() == true)
  {
    pScreen->drawString("Kernel mode", nLine+1, nCols+1, DebuggerIO::Yellow, DebuggerIO::Black);
  }
  else
  {
    pScreen->drawString("Kernel mode", nLine+1, nCols+1, DebuggerIO::Yellow, DebuggerIO::Black);
  }
  nLine++;
  
  for (size_t i = 0; i < state.getRegisterCount(); i++)
  {
    LargeStaticString str;
    str = state.getRegisterName(i);
    pScreen->drawString(str, nLine+1, nCols+1, DebuggerIO::Yellow, DebuggerIO::Black);
#ifdef BITS_32
    const unsigned int k_nRightOffset = 10;
#endif
#ifdef BITS_64
    const unsigned int k_nRightOffset = 18;
#endif
    str = "0x";
    str.append(state.getRegister(i), 16, state.getRegisterSize(i) * 2, '0');
    pScreen->drawString(str, nLine+1, pScreen->getWidth()-k_nRightOffset, DebuggerIO::White, DebuggerIO::Black);
    nLine++;
    if (nLine == nLines) break;
  }
}

void TraceCommand::drawStacktrace(int nLines, DebuggerIO *pScreen, InterruptState &state)
{
  Backtrace bt;
  bt.performBacktrace(state.getBasePointer(), state.getInstructionPointer());

  HugeStaticString output;
  bt.prettyPrint(output, nLines-1);
  pScreen->drawString(output, pScreen->getHeight()-nLines, 0, DebuggerIO::White, DebuggerIO::Black);
}
