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

#include "TUI.h"
#include <Module.h>
#include "../../kernel/core/BootIO.h"
#include <machine/Machine.h>
#include <machine/Keyboard.h>
#include <processor/Processor.h>

extern BootIO bootIO;

TUI::TUI() :
  m_Echo(true), m_EchoNewlines(true), m_EchoBackspace(true)
{
}

TUI::~TUI()
{
}

uint64_t TUI::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                             uint64_t p6, uint64_t p7, uint64_t p8)
{
  // Expect a request from Console.
  uint64_t operation = p1;
  uint64_t param = p2;
  uint64_t size = p3;
  uint64_t buffer = p4;

  HugeStaticString str;

  if (operation == CONSOLE_WRITE)
  {
    // For the moment just forward to BootIO - THIS WILL CHANGE!
    char *buf = new char[size+1];
    memcpy(reinterpret_cast<void*>(buf), reinterpret_cast<void*>(buffer), size);
    buf[size] = '\0';

    str.append(buf);

    delete [] buf;

    bootIO.write(str, BootIO::LightGreen, BootIO::Black);
    return size;
  }
  else if (operation == CONSOLE_READ)
  {
    // Read - forward to keyboard. THIS WILL CHANGE!
    Keyboard *pK = Machine::instance().getKeyboard();
    if (!pK)
    {
      WARNING("TUI: No keyboard present!");
      return 0;
    }

    char *buf = reinterpret_cast<char*>(buffer);
    int i = 0;
    char c = 0;

    while ( i < size )
    {
      c = pK->getCharNonBlock();
      if (c == 0)
      {
        if (i == 0)
          continue;
        else
        {
          return i;
        }
      }

      // Echo.
      if (m_Echo)
      {
        if ( (m_EchoNewlines && c == '\n') ||
             (m_EchoBackspace && c == 0x08) ||
             (c != '\n' && c != 0x08) )
        {
          str.append(c);
          bootIO.write(str, BootIO::LightGreen, BootIO::Black);
          str.clear();
        }
      }

      buf[i++] = c;
    }
    return i;
  }
  else if (operation == CONSOLE_SETATTR)
  {
    m_Echo = static_cast<bool>(p3);
    m_EchoNewlines = static_cast<bool>(p4);
    m_EchoBackspace = static_cast<bool>(p5);
    return 0;
  }
  else if (operation == CONSOLE_GETATTR)
  {
    bool *echo = reinterpret_cast<bool*>(p3);
    *echo = m_Echo;
    bool *echoNewlines = reinterpret_cast<bool*>(p4);
    *echoNewlines = m_EchoNewlines;
    bool *echoBackspace = reinterpret_cast<bool*>(p5);
    *echoBackspace = m_EchoBackspace;
    return 0;
  }
  else if (operation == CONSOLE_GETCOLS)
  {
    /// \todo Implement.
    return 80;
  }
  else if (operation == CONSOLE_GETROWS)
  {
    /// \todo Implement.
    return 25;
  }
  return 0;
}

void init()
{
  TUI *pTui = new TUI();
  pTui->initialise();
  ConsoleManager::instance().registerConsole(String("console0"), pTui, 0);
}

void destroy()
{
}

MODULE_NAME("TUI");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
MODULE_DEPENDS("console");
