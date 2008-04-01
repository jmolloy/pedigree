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
#include <machine/Timer.h>
#include <machine/Machine.h>
#include <utilities/utility.h>

Log Log::m_Instance;

Log::Log () :
  m_nEntries(0),
  m_Buffer(),
  m_NumberType(Dec)
{
}

Log::~Log ()
{
}

Log &Log::operator<< (const char *str)
{
  // Ensure that we don't overrun the buffer.
  int nExtraCharsAllowed = LOG_LENGTH - strlen(m_Buffer.str);
  
  strncat(m_Buffer.str, str, nExtraCharsAllowed);
  return *this;
}

Log &Log::operator<< (int n)
{
  if (m_NumberType == Dec)
    writeDec( static_cast<unsigned int>(n));
  else
    writeHex( static_cast<unsigned int>(n));
  return *this;
}

Log &Log::operator<< (Modifier type)
{
  // Flush the buffer.
  if (type == Flush && m_nEntries < LOG_ENTRIES)
    m_pLog[m_nEntries++] = m_Buffer;

  return *this;
}

Log &Log::operator<< (NumberType type)
{
  m_NumberType = type;
  return *this;
}

Log &Log::operator<< (SeverityLevel level)
{
  switch (level)
  {
  case Fatal:
  case Error:
  case Warning:
  case Notice:
    // Zero the buffer.
    m_Buffer.str[0] = '\0';
    m_Buffer.type = level;

    Machine &machine = Machine::instance();
    if (machine.isInitialised() == true && machine.getTimer() != 0)
    {
      Timer &timer = *machine.getTimer();
      m_Buffer.timestamp = timer.getTickCount();
    }
    else
      m_Buffer.timestamp = 0;
    break;
  }
  return *this;
}

void Log::writeHex(unsigned int n)
{
  unsigned int tmp;

  strcat(m_Buffer.str, "0x");

  char tmpStr[2];
  tmpStr[1] = '\0';
  
  bool noZeroes = true;

  int i;
  for (i = sizeof(unsigned int)*8-4; i > 0; i -= 4)
  {
    tmp = (n >> i) & 0xF;
    if (tmp == 0 && noZeroes)
      continue;
    
    if (tmp >= 0xA)
    {
      noZeroes = false;
      tmpStr[0] = tmp-0xA+'a';
      strcat(m_Buffer.str, tmpStr);
    }
    else
    {
      noZeroes = false;
      tmpStr[0] = tmp+'0';
      strcat(m_Buffer.str, tmpStr);
    }
  }
  
  tmp = n & 0xF;
  if (tmp >= 0xA)
  {
    tmpStr[0] = tmp-0xA+'a';
    strcat(m_Buffer.str, tmpStr);
  }
  else
  {
    tmpStr[0] = tmp+'0';
    strcat(m_Buffer.str, tmpStr);
  }
}

void Log::writeDec(unsigned int n)
{
  if (n == 0)
  {
    strcat(m_Buffer.str, "0");
    return;
  }
  
  unsigned int acc = n;
  char c[32];
  int i = 0;
  while (acc > 0)
  {
    c[i] = '0' + acc%10;
    acc /= 10;
    i++;
  }
  c[i] = 0;

  char c2[32];
  c2[i--] = 0;
  int j = 0;
  while(i >= 0)
  {
    c2[i--] = c[j++];
  }
  strcat(m_Buffer.str, c2);
}
