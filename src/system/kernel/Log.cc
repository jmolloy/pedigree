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

#include <Log.h>
#include <panic.h>
#include <machine/Timer.h>
#include <machine/Machine.h>
#include <utilities/utility.h>
#include <processor/Processor.h>

Log Log::m_Instance;

Log::Log () :
  m_DynamicLog(),
  m_StaticEntries(0),
  m_Buffer(),
  m_NumberType(Dec)
{
}

Log::~Log ()
{
}

Log &Log::operator<< (const char *str)
{
  m_Buffer.str.append(str);
  return *this;
}

Log &Log::operator<< (bool b)
{
  if (b)
    return *this << "true";

  return *this << "false";
}

template<class T>
Log &Log::operator << (T n)
{
  size_t radix = 10;
  if (m_NumberType == Hex)
  {
    radix = 16;
    m_Buffer.str.append("0x");
  }
  m_Buffer.str.append(n, radix);
  return *this;
}

// NOTE: Make sure that the templated << operator gets only instanciated for
//       integer types.
template Log &Log::operator << (char);
template Log &Log::operator << (unsigned char);
template Log &Log::operator << (short);
template Log &Log::operator << (unsigned short);
template Log &Log::operator << (int);
template Log &Log::operator << (unsigned int);
template Log &Log::operator << (long);
template Log &Log::operator << (unsigned long);
// NOTE: Instantiating these for MIPS32 requires __udiv3di, but we only have
//       __udiv3ti (??) in libgcc.a for mips.
#ifndef MIPS32
template Log &Log::operator << (long long);
template Log &Log::operator << (unsigned long long);
#endif

Log &Log::operator<< (Modifier type)
{
  // Flush the buffer.
  if (type == Flush)
  {
    if (m_StaticEntries >= LOG_ENTRIES)
    {
      if (Processor::isInitialised() == 0)
        panic("Log: Not enough static log entries");

      DynamicLogEntry *entry = new DynamicLogEntry;
      entry->type = m_Buffer.type;
      entry->timestamp = m_Buffer.timestamp;
      entry->str = m_Buffer.str;
      m_DynamicLog.pushBack(entry);
    }
    else
      m_StaticLog[m_StaticEntries++] = m_Buffer;
  }

  return *this;
}

Log &Log::operator<< (NumberType type)
{
  m_NumberType = type;
  return *this;
}

Log &Log::operator<< (SeverityLevel level)
{
  // Zero the buffer.
  m_Buffer.str.clear();
  m_Buffer.type = level;

  Machine &machine = Machine::instance();
  if (machine.isInitialised() == true &&
      machine.getTimer() != 0)
  {
    Timer &timer = *machine.getTimer();
    m_Buffer.timestamp = timer.getTickCount();
  }
  else
    m_Buffer.timestamp = 0;

  return *this;
}
