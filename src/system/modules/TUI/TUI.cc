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
#include "Vt100.h"

extern BootIO bootIO;

TUI *g_pTui;
Vt100 *g_pVt100 = 0;

TUI::TUI() :
  m_Echo(true), m_EchoNewlines(true), m_EchoBackspace(true), m_bIsTextMode(true), m_QueueLength(0)
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
    char *buf = new char[size+1];
    memcpy(reinterpret_cast<void*>(buf), reinterpret_cast<void*>(buffer), size);
    buf[size] = '\0';

    if (m_bIsTextMode)
    {
      // For the moment just forward to BootIO
      str.append(buf);

      delete [] buf;

      bootIO.write(str, BootIO::LightGreen, BootIO::Black);
      return size;
    }
    else
    {
      // In a graphics mode - forward to the VT100 object.
      g_pVt100->write(buf);

      delete [] buf;
      return size;
    }
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
    char c;
    Keyboard::Character ch;

    while ( i < size )
    {
      if (m_QueueLength)
      {
        buf[i++] = m_pQueue[0];
        for (int j = 0; j < m_QueueLength-1; j++)
        {
          m_pQueue[j] = m_pQueue[j+1];
        }
        m_QueueLength--;
        continue;
      }

      ch = pK->getCharacterNonBlock();
      if (ch.valid == 0)
      {
        if (i == 0)
          continue;
        else
        {
          return i;
        }
      }
      if (ch.is_special)
      {
        switch (ch.value)
        {
          case KB_ARROWLEFT:
            c = '\e';
            m_pQueue[m_QueueLength++] = '[';
            //m_pQueue[m_QueueLength++] = '1';
            m_pQueue[m_QueueLength++] = 'D';
            break;
          case KB_ARROWRIGHT:
            c = '\e';
            m_pQueue[m_QueueLength++] = '[';
            //m_pQueue[m_QueueLength++] = '1';
            m_pQueue[m_QueueLength++] = 'C';
            break;
          case KB_ARROWUP:
            c = '\e';
            m_pQueue[m_QueueLength++] = '[';
            //m_pQueue[m_QueueLength++] = '1';
            m_pQueue[m_QueueLength++] = 'A';
            break;
          case KB_ARROWDOWN:
            c = '\e';
            m_pQueue[m_QueueLength++] = '[';
            //m_pQueue[m_QueueLength++] = '1';
            m_pQueue[m_QueueLength++] = 'B';
            break;
        }
      }
      else
      {
        c = ch.value;
      }

      // Echo.
      if (m_Echo)
      {
        if ( (m_EchoNewlines && c == '\n') ||
             (m_EchoBackspace && c == 0x08) ||
             (c != '\n' && c != 0x08) )
        {
          if (m_bIsTextMode)
          {
            str.append(c);
            bootIO.write(str, BootIO::LightGreen, BootIO::Black);
            str.clear();
          }
          else
          {
            g_pVt100->write(c);
          }
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

void TUI::modeChanged(Display::ScreenMode mode, void *pFramebuffer)
{
  m_Mode = mode;

  g_pVt100 = new Vt100(mode, pFramebuffer);

  m_bIsTextMode = false;
}

void init()
{
  g_pTui = new TUI();
  g_pTui->initialise();
  ConsoleManager::instance().registerConsole(String("console0"), g_pTui, 0);
}

void destroy()
{
}

MODULE_NAME("TUI");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
MODULE_DEPENDS("console");
