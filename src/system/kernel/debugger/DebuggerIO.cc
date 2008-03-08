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
#include <DebuggerIO.h>
#include <DebuggerCommand.h>
#include <utilities/utility.h>

bool DebuggerIO::readCli(HugeStaticString &str, DebuggerCommand *pAutoComplete)
{
  // Are we ready to recieve another command?
  if (m_bReady)
  {
    // Yes, output a prompt...
    writeCli("(db) ", DebuggerIO::LightGrey, DebuggerIO::Black);
    // ...and zero the command string.
    m_pCommand[0] = '\0';
    m_bReady = false;
  }

  char ch = 0;
  // Spin in a loop until we get a printable character. getChar returns 0 if a non-printing
  // character is recieved.
  while ( !(ch = getChar()) )
    ;

  // Was this a newline?
  if (ch == '\n' || ch == '\r')
  {
    // Command finished, we're ready for the next.
    m_bReady = true;
    writeCli("\n", DebuggerIO::White, DebuggerIO::Black);
  }
  else
  {
    // We've got a character, try and append it to our command string.
    // But first, was it a backspace?
    if (ch == 0x08)
    {
      // Try and erase one letter of the command string.
      if (strlen(m_pCommand))
      {
        m_pCommand[strlen(m_pCommand)-1] = '\0';
      
        writeCli(ch, DebuggerIO::White, DebuggerIO::Black);
      }
    }
    // Tab?
    else if (ch == 0x09)
    {
      if (pAutoComplete)
      {
        // Get the full autocomplete string.
        const char *pACString = pAutoComplete->getString();
        // HACK:: Here we hack like complete bitches. Just find the last space in the string,
        // and memcpy the full autocomplete string in.
        int i;
        for (i = strlen(m_pCommand); i >= 0; i--)
          if (m_pCommand[i] == ' ')
            break;
        
        // We also haxxor the cursor, by writing loads of backspaces, then rewriting the whole string.
        int nBackspaces = strlen(m_pCommand)-i;
        for (int j = 0; j < nBackspaces-1; j++)
          putChar('\x08' /* backspace */, DebuggerIO::White, DebuggerIO::Black);
        
        writeCli(pACString, DebuggerIO::White, DebuggerIO::Black);
        
        // Memcpy the full autocomplete string in.
        memcpy(&m_pCommand[i+1], pACString, strlen(pACString)+1);
      }
    }
    else
    {
      // Normal, printing character.
      int len = strlen(m_pCommand);
      if (len < COMMAND_MAX-1)
      {
        // Add it to the command string, and null terminate.
        m_pCommand[len] = ch;
        m_pCommand[len+1] = '\0';
        // And echo it back to the screen, too.
        writeCli(ch, DebuggerIO::White, DebuggerIO::Black);
      }
    }
  }

  // Now do a strncpy to the target string.
  str = static_cast<const char *>(m_pCommand);
  
  return m_bReady;
}

void DebuggerIO::writeCli(const char *str, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
  // We want to disable refreshes during writing, so the screen only gets updated once.
  bool bRefreshWasEnabled = false;
  if (m_bRefreshesEnabled)
  {
    bRefreshWasEnabled = true;
    m_bRefreshesEnabled = false;
  }

  // For every character, call writeCli.
  while (*str)
    writeCli(*str++, foreColour, backColour);

  // If refreshes were enabled to start with, reenable them, and force a refresh.
  if (bRefreshWasEnabled)
  {
    m_bRefreshesEnabled = true;
    forceRefresh();
  }
}

void DebuggerIO::writeCli(char c, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
  // Write the character to the current cursor position.
  putChar(c, foreColour, backColour);
  
  // Scroll if required.
  scroll();

  // Move the cursor.
  moveCursor();

  if (m_bRefreshesEnabled)
    forceRefresh();

}
