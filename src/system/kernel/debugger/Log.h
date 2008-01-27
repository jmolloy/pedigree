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

#ifndef DEBUGGER_LOG_H
#define DEBUGGER_LOG_H

// Global Log object defined in main.
#include <main.h>

#define NOTICE(text) \
  do \
  { \
    g_Log << Log::Notice << text << Endl; \
  }

#define WARNING(text) \
  do \
  { \
    g_Log << Log::Warning << text << Endl; \
  }

#define ERROR(text) \
  do \
  { \
    g_Log << Log::Error << text << Endl; \
  }

#define FATAL(text) \
  do \
  { \
    g_Log << Log::Fatal << text << Endl; \
  }

enum NumberType
{
  Hex, Dec
};
#define Endl "\n"

class Log
{
public:
  enum SeverityLevel
  {
    Notice,
    Warning,
    Error,
    Fatal
  }

  /**
   * Default constructor - does nothing.
   */
  Log ();
  ~Log ();
  
  /**
   * Adds an entry to the log.
   */
  Log &operator<< (const char *str);
  Log &operator<< (SeverityLevel level);
  Log &operator<< (NumberType type);
  Log &operator<< (int n);
};

#endif
