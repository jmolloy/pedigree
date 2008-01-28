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
    g_Log << Log::Notice << text << Log::End; \
  }

#define WARNING(text) \
  do \
  { \
    g_Log << Log::Warning << text << Log::End; \
  }

#define ERROR(text) \
  do \
  { \
    g_Log << Log::Error << text << Log::End; \
  }

#define FATAL(text) \
  do \
  { \
    g_Log << Log::Fatal << text << Log::End; \
  }

/// The maximum length of an individual log entry.
/// \todo Change to using dynamic memory.
#define LOG_LENGTH  128
/// The maximum number of entries in the log.
/// \todo Change to using dynamic memory.
#define LOG_ENTRIES 64

enum NumberType
{
  Hex, Dec
};

class Log
{
public:
  enum SeverityLevel
  {
    Notice,
    Warning,
    Error,
    Fatal,
    End
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
  /**
   * Adds an entry to the log.
   */
  Log &operator<< (int n);
  /**
   * Starts an entry in the log (or stops, if level == SeverityLevel::End).
   */
  Log &operator<< (SeverityLevel level);
  /**
   * Changes the number type between hex and decimal.
   */
  Log &operator<< (NumberType type);

private:
  /**
   * Buffer of log messages.
   * \todo Make this a dynamic vector.
   */
  
};

#endif
