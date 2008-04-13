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
#include "MemoryInspector.h"
#include <processor/VirtualAddressSpace.h>

MemoryInspector::MemoryInspector()
    : DebuggerCommand(), Scrollable(), m_nCharsPerLine(8)
{
}


MemoryInspector::~MemoryInspector()
{
}

void MemoryInspector::autocomplete (const HugeStaticString& input, HugeStaticString& output)
{
  output = "<Address> | <Register name>";
}

bool MemoryInspector::execute (const HugeStaticString& input, HugeStaticString& output, InterruptState& state, DebuggerIO* pScreen)
{
  // Is the terminal wide enough to do 16-character wide display?
  if (pScreen->getWidth() >= 87)
    m_nCharsPerLine = 16;
  if (pScreen->getWidth() >= 138)
    m_nCharsPerLine = 32;

  // Let's enter 'raw' screen mode.
  pScreen->disableCli();

  // Initialise the Scrollable class
  move(0, 1);
  resize(pScreen->getWidth(), pScreen->getHeight() - 3);
  setScrollKeys('j', 'k');

  // Clear the top status lines.
  pScreen->drawHorizontalLine(' ',
                              0,
                              0,
                              pScreen->getWidth() - 1,
                              DebuggerIO::White,
                              DebuggerIO::Green);
  
  // Write the correct text in the upper status line.
  pScreen->drawString("Pedigree debugger - Memory inspector",
                      0,
                      0,
                      DebuggerIO::White,
                      DebuggerIO::Green);

  resetStatusLine(pScreen);
  
  // Main loop.
  bool bStop = false;
  while(!bStop)
  {
    refresh(pScreen);

    // Wait for input.
    char c;
    while( (c=pScreen->getChar()) == 0)
      ;

    // TODO: Use arrow keys and page up/down someday
    if (c == 'j')
      scroll(-1);
    else if (c == 'k')
      scroll(1);
    else if (c == ' ')
      scroll(static_cast<ssize_t>(height()));
    else if (c == 0x08)
      scroll(-static_cast<ssize_t>(height()));
    else if (c == 'q')
      bStop = true;
    else if (c == 'g')
      doGoto(pScreen, state);
    else if (c == '/')
      doSearch(true, pScreen, state);
    else if (c == '?')
      doSearch(false, pScreen, state);
  }

  // HACK:: Serial connections will fill the screen with the last background colour used.
  //        Here we write a space with black background so the CLI screen doesn't get filled
  //        by some random colour!
  pScreen->drawString(" ", 1, 0, DebuggerIO::White, DebuggerIO::Black);
  pScreen->enableCli();
  return true;
}

const char *MemoryInspector::getLine1(size_t index, DebuggerIO::Colour & colour, DebuggerIO::Colour & bgColour)
{
  colour = DebuggerIO::DarkGrey;
  uintptr_t nLine = index*m_nCharsPerLine;
  static NormalStaticString str;
  str.clear();
  str.append(nLine, 16, sizeof(uintptr_t)*2, '0');
  return str;
}

const char *MemoryInspector::getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour)
{
  colOffset = sizeof(uintptr_t)*2+1;
  // Get the line we want.
  uintptr_t nLine = index*m_nCharsPerLine;
  // Is it paged in?
#if !defined(MIPS_COMMON) && !defined(ARM_COMMON)
  if (!VirtualAddressSpace::getKernelAddressSpace().isMapped(reinterpret_cast<void*> (nLine)))
  {
    colour = DebuggerIO::Red;
    return "Memory not mapped.";
  }
  else
  {
#endif
    uint8_t *pLine = reinterpret_cast<uint8_t*> (nLine);
    static LargeStaticString str;
    str.clear();
    for (unsigned int i = 0; i < m_nCharsPerLine; i++)
    {
      str.append(pLine[i], 16, 2, '0');
      str += ' ';
      if (((i+1) % 8 == 0) && ((i+1) < m_nCharsPerLine))
        str += "- ";
    }

    str += "  ";
    for (unsigned int i = 0; i < m_nCharsPerLine; i++)
    {
      if (pLine[i] >= 33 && pLine[i] <= 126)
        str.append(static_cast<char> (pLine[i]), 1);
      else
        str += '.';
    }

    return str;
#if !defined(MIPS_COMMON) && !defined(ARM_COMMON)
  }
#endif
  return 0;
}

size_t MemoryInspector::getLineCount()
{
  // We have as many lines as the entirety of the address space / 8.
  // TODO: Please verify that this is correct, JamesM :-)
  #if defined(BITS_32)
    return 0x20000000;
  #elif defined(BITS_64)
    return 0x2000000000000000;
  #endif
}

void MemoryInspector::resetStatusLine(DebuggerIO *pScreen)
{
  // Clear the bottom status lines.
  // TODO: If we use arrow keys and page up/down keys we actually can remove the status line
  //       because the interface is then intuitive enough imho.
  pScreen->drawHorizontalLine(' ',
                              pScreen->getHeight() - 1,
                              0,
                              pScreen->getWidth() - 1,
                              DebuggerIO::White,
                              DebuggerIO::Green);

  // Write some helper text in the lower status line.
  // TODO FIXME: Drawing this might screw the top status bar
  pScreen->drawString("backspace: Page up. space: Page down. q: Quit. /,?: Fwd/Rev search. g: Goto.",
                      pScreen->getHeight()-1, 0, DebuggerIO::White, DebuggerIO::Green);
  pScreen->drawString("backspace", pScreen->getHeight()-1, 0, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("space", pScreen->getHeight()-1, 20, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("q", pScreen->getHeight()-1, 38, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("/,?", pScreen->getHeight()-1, 47, DebuggerIO::Yellow, DebuggerIO::Green);
  pScreen->drawString("g", pScreen->getHeight()-1, 68, DebuggerIO::Yellow, DebuggerIO::Green);
}

void MemoryInspector::doGoto(DebuggerIO *pScreen, InterruptState &state)
{
  static LargeStaticString str;
  str.clear();
  while (1)
  {
    // Erase the bottom line.
    pScreen->drawHorizontalLine(' ', pScreen->getHeight()-2, 0, pScreen->getWidth()-1, DebuggerIO::White, DebuggerIO::Black);
    // Print helper text.
    pScreen->drawString("[Go to address or register value]", pScreen->getHeight()-2, 0, DebuggerIO::White, DebuggerIO::Black);
    // Print current string contents.
    pScreen->drawString(str, pScreen->getHeight()-2, 35, DebuggerIO::White, DebuggerIO::Black);
    // Get a character.
    char c;
    while( (c=pScreen->getChar()) == 0)
      ;
    if (c == '\n' || c == '\r')
      break;
    else if (c == 0x08)
      str.stripLast();
    else
      str += c;
  }

  pScreen->drawHorizontalLine(' ', pScreen->getHeight()-2, 0, pScreen->getWidth()-1, DebuggerIO::White, DebuggerIO::Black);
  
  uintptr_t nAddress;
  if (!tryGoto(str, nAddress, state))
  {
    pScreen->drawString("Bad address or register.", pScreen->getHeight()-2, 0, DebuggerIO::Red, DebuggerIO::Black);
  }
  else
  {
    scrollTo(nAddress/m_nCharsPerLine);
  }
}

void MemoryInspector::doSearch(bool bForward, DebuggerIO *pScreen, InterruptState &state)
{
  static LargeStaticString str;
  static LargeStaticString str2;
  str.clear();
  while (1)
  {
    // Erase the bottom line.
    pScreen->drawHorizontalLine(' ', pScreen->getHeight()-2, 0, pScreen->getWidth()-1, DebuggerIO::White, DebuggerIO::Black);
    // Print helper text.
    if (bForward)
      pScreen->drawString("[Search] ", pScreen->getHeight()-2, 0, DebuggerIO::White, DebuggerIO::Black);
    else
      pScreen->drawString("[Reverse]", pScreen->getHeight()-2, 0, DebuggerIO::White, DebuggerIO::Black);
    // Print current string contents.
    pScreen->drawString(str, pScreen->getHeight()-2, 10, DebuggerIO::White, DebuggerIO::Black);
    // Get a character.
    char c;
    while( (c=pScreen->getChar()) == 0)
      ;
    if (c == '\n' || c == '\r')
      break;
    else if (c == 0x08)
      str.stripLast();
    else
      str += c;
  }
  
  pScreen->drawHorizontalLine(' ', pScreen->getHeight()-2, 0, pScreen->getWidth()-1, DebuggerIO::White, DebuggerIO::Black);
  
  unsigned int nChars = 0;
  uint8_t pChars[8];
  
  // Valid search?
  if (str[0] == '"')
  {
    // Ascii search.
    str.stripFirst();
    str.stripLast();

    if (str.length() > 8)
    {
      pScreen->drawString("Search string too long (> 8 characters)", pScreen->getHeight()-2, 0, DebuggerIO::Red, DebuggerIO::Black);
      return;
    }
    
    nChars = str.length();
    for (size_t i = 0; i < str.length(); i++)
      pChars[i] = str[i];
  }
  else
  {
#ifdef BIG_ENDIAN
#error Big endian people will have problems here.
#endif
    // Assume hex integer.
    if (str.left(2) == "0x")
      str.stripFirst(2);
    nChars = str.length()/2;
    if (nChars >= 8)
    {
      pScreen->drawString("Hex string too long, 8 bytes max.", pScreen->getHeight()-2, 0, DebuggerIO::Red, DebuggerIO::Black);
      return;
    }
    for (size_t i = 0; i < nChars; i++)
    {
      // Get our nibbles.
      TinyStaticString mystr;
      mystr += str[i*2];
      mystr += str[i*2+1];
      // Convert.
      int nConverted = mystr.intValue(16);
      // Failed?
      if (nConverted == -1)
      {
        pScreen->drawString("Malformed hexadecimal number.", pScreen->getHeight()-2, 0, DebuggerIO::Red, DebuggerIO::Black);
        return;
      }
      pChars[nChars-i-1] = static_cast<uint8_t> (nConverted);
    }
  }
  
  if (bForward)
    str = "[Search for ";
  else 
    str = "[Reverse for ";
  for (size_t i = 0; i < nChars; i++)
  {
    str.append(pChars[i], 16, 2, '0');
    str += ' ';
  }
  str += ": over ?(K|M) of memory]";
  pScreen->drawString(str, pScreen->getHeight()-2, 0, DebuggerIO::White, DebuggerIO::Black);
  
  str2.clear();
  while (1)
  {
    // Erase the bottom line.
    pScreen->drawHorizontalLine(' ', pScreen->getHeight()-2, 0, pScreen->getWidth()-1, DebuggerIO::White, DebuggerIO::Black);
    // Print helper text.
    pScreen->drawString(str, pScreen->getHeight()-2, 0, DebuggerIO::White, DebuggerIO::Black);
    // Print current string contents.
    pScreen->drawString(str2, pScreen->getHeight()-2, str.length()+1, DebuggerIO::White, DebuggerIO::Black);
    // Get a character.
    char c;
    while( (c=pScreen->getChar()) == 0)
      ;
    if (c == '\n' || c == '\r')
      break;
    else if (c == 0x08)
      str2.stripLast();
    else
      str2 += c;
  }
  
  // Erase the bottom line.
  pScreen->drawHorizontalLine(' ', pScreen->getHeight()-2, 0, pScreen->getWidth()-1, DebuggerIO::White, DebuggerIO::Black);
  str = str2.right(1);
  char suffix = str[0];
  if (suffix == 'M' || suffix == 'K')
    str2.stripLast();
  int num = str2.intValue();
  if (num == -1)
  {
    pScreen->drawString("Malformed number.", pScreen->getHeight()-2, 0, DebuggerIO::Red, DebuggerIO::Black);
    return;
  }
  size_t nRange = static_cast<size_t> (num);
  if (suffix == 'K')
    nRange *= 1024;
  else if (suffix == 'M')
    nRange *= 1048576;
  
  pScreen->drawString("Searching...", pScreen->getHeight()-2, 0, DebuggerIO::Green, DebuggerIO::Black);
  
  // Do the search.
  
}

bool MemoryInspector::tryGoto(LargeStaticString &str, uintptr_t &result, InterruptState &state)
{
  for (size_t i = 0;i < state.getRegisterCount();i++)
  {
    LargeStaticString reg(state.getRegisterName(i));
    if (reg == str)
    {
      result = state.getRegister(i);
      return true;
    }
  }
  int n = str.intValue();
  if (n == -1)
    return false;
  else
  {
    result = static_cast<uintptr_t> (n);
    return true;
  }
}
