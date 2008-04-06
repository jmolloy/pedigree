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

Log &Log::operator<< (Modifier type)
{
  // Flush the buffer.
  if (type == Flush)
  {
    if (m_StaticEntries >= LOG_ENTRIES)
    {
      if (Processor::isInitialised() != 0)
      {
        DynamicLogEntry *entry = new DynamicLogEntry;
        entry->type = m_Buffer.type;
        entry->timestamp = m_Buffer.timestamp;
        entry->str = m_Buffer.str;
        m_DynamicLog.pushBack(entry);
      }
      else
      {
        panic("Log: Not enough static log entries");
      }
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
