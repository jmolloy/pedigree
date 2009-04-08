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
      g_pVt100->setNewlineNLCR(m_NlCausesCr);
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
        c = m_pQueue[0];
        if (c == '\n') c = '\r';
        buf[i++] = c;
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
          return i;
      }
      if (ch.is_special)
      {
        switch (ch.value)
        {
          case KB_ARROWLEFT:
            c = 0x1B;
            m_pQueue[m_QueueLength++] = '[';
            //m_pQueue[m_QueueLength++] = 'O';
            m_pQueue[m_QueueLength++] = 'D';
            break;
          case KB_ARROWRIGHT:
            c = '\e';
            m_pQueue[m_QueueLength++] = '[';
            //m_pQueue[m_QueueLength++] = 'O';
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
      else if (ch.ctrl)
      {
        switch (ch.value)
        {
          case ' ': c = 0;  break;
          case 'a':
          case 'b':
          case 'c':
          case 'd':
          case 'e':
          case 'f':
          case 'g':
          case 'h':
          case 'i':
          case 'j':
          case 'k':
          case 'l':
          case 'm':
          case 'n':
          case 'o':
          case 'p':
          case 'q':
          case 'r':
          case 's':
          case 't':
          case 'u':
          case 'v':
          case 'w':
          case 'x':
          case 'y':
          case 'z':
            c = ch.value - 'a' + 1;
            break;
          default:
            c = ch.value;
            break;
        }
      }
      else if (ch.alt)
      {
        c = '\e';
        m_pQueue[m_QueueLength++] = ch.value | 0x80;
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
      // ENTER should actually be \r. Perform this mapping no matter what,
      // before we perform the NL->CR / CR->NL mappings.
      if (c == '\n') c = '\r';

      if (c == '\n' && m_MapNlToCrIn) c = '\r';
      else if (c == '\r' && m_MapCrToNlIn) c = '\n';
      buf[i++] = c;
    }

    return i;
  }
  else if (operation == CONSOLE_SETATTR)
  {
    m_Echo = static_cast<bool>(p3);
    m_EchoNewlines = static_cast<bool>(p4);
    m_EchoBackspace = static_cast<bool>(p5);
    m_NlCausesCr = static_cast<bool>(p6);

    m_MapNlToCrIn = static_cast<bool>(p7);
    m_MapCrToNlIn = static_cast<bool>(p8);
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
    bool *nlCausesCr = reinterpret_cast<bool*>(p6);
    *nlCausesCr = m_NlCausesCr;
    bool *mapNlToCrIn = reinterpret_cast<bool*>(p7);
    *mapNlToCrIn = m_MapNlToCrIn;
    bool *mapCrToNlIn = reinterpret_cast<bool*>(p8);
    *mapCrToNlIn = m_MapCrToNlIn;
    return 0;
  }
  else if (operation == CONSOLE_GETCOLS)
  {
    if (m_bIsTextMode)
      return 80;
    else
      return g_pVt100->m_nWidth;
  }
  else if (operation == CONSOLE_GETROWS)
  {
    if (m_bIsTextMode)
      return 25;
    else
      return g_pVt100->m_nHeight;
  }
  else if (operation == CONSOLE_DATA_AVAILABLE)
  {
    // Read - forward to keyboard. THIS WILL CHANGE!
    Keyboard *pK = Machine::instance().getKeyboard();
    if (!pK)
    {
      WARNING("TUI: No keyboard present!");
      return 0;
    }
    
    /// \todo Clean this up! Redundancy isn't cool!
    /// \note This is required so that if a read() call isn't made and keys have
    ///       been pressed we still find out that keys are available. Then the
    ///       read() call successfully reads from the queue.
    if(m_QueueLength == 0)
    {
      Keyboard::Character ch;
      ch = pK->getCharacterNonBlock();
      if (ch.valid != 0)
      {
        if (ch.is_special)
        {
          switch (ch.value)
          {
            case KB_ARROWLEFT:
              m_pQueue[m_QueueLength++] = 0x1B;
              m_pQueue[m_QueueLength++] = '[';
              m_pQueue[m_QueueLength++] = 'D';
              break;
            case KB_ARROWRIGHT:
              m_pQueue[m_QueueLength++] = '\e';
              m_pQueue[m_QueueLength++] = '[';
              m_pQueue[m_QueueLength++] = 'C';
              break;
            case KB_ARROWUP:
              m_pQueue[m_QueueLength++] = '\e';
              m_pQueue[m_QueueLength++] = '[';
              m_pQueue[m_QueueLength++] = 'A';
              break;
            case KB_ARROWDOWN:
              m_pQueue[m_QueueLength++] = '\e';
              m_pQueue[m_QueueLength++] = '[';
              m_pQueue[m_QueueLength++] = 'B';
              break;
          }
        }
        else if (ch.ctrl)
        {
          switch (ch.value)
          {
            case ' ': m_pQueue[m_QueueLength++] = 0;  break;
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'k':
            case 'l':
            case 'm':
            case 'n':
            case 'o':
            case 'p':
            case 'q':
            case 'r':
            case 's':
            case 't':
            case 'u':
            case 'v':
            case 'w':
            case 'x':
            case 'y':
            case 'z':
              m_pQueue[m_QueueLength++] = ch.value - 'a' + 1;
              break;
            default:
              m_pQueue[m_QueueLength++] = ch.value;
              break;
          }
        }
        else if (ch.alt)
        {
          m_pQueue[m_QueueLength++] = '\e';
          m_pQueue[m_QueueLength++] = ch.value | 0x80;
        }
        else
          m_pQueue[m_QueueLength++] = ch.value;
      }
    }
    
    return (m_QueueLength > 0);
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
