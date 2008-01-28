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

#define NOTICE(text) \
  do \
  { \
    g_Log << Log::Notice << text << Log::End; \
  } while(0);

#define WARNING(text) \
  do \
  { \
    g_Log << Log::Warning << text << Log::End; \
  } while(0);

#define ERROR(text) \
  do \
  { \
    g_Log << Log::Error << text << Log::End; \
  } while(0);

#define FATAL(text) \
  do \
  { \
    g_Log << Log::Fatal << text << Log::End; \
  } while(0);

/// The maximum length of an individual log entry.
/// \todo Change to using dynamic memory.
#define LOG_LENGTH  128
/// The maximum number of entries in the log.
/// \todo Change to using dynamic memory.
#define LOG_ENTRIES 64

extern class Log g_Log;

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
  };
  
  /**
   * Stores an entry in the log.
   */
  typedef struct LogEntry
  {
    unsigned int timestamp; ///< The time (since boot) that this log entry was added, in ticks.
    SeverityLevel type;     ///< The severity level of this entry.
    char str[LOG_LENGTH];   ///< The actual entry text. \todo Change this to using dynamic memory.
  } LogEntry_t;

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
   * \todo This function should gain and release spinlocks, depending on level.
   */
  Log &operator<< (SeverityLevel level);
  /**
   * Changes the number type between hex and decimal.
   */
  Log &operator<< (NumberType type);

  /**
   * Returns the n'th log entry, counting from the start.
   */
  LogEntry_t getEntry(int n)
  {
    return m_pLog[n];
  }
  
  /**
   * Returns the number of entries in the log.
   */
  int getEntryCount()
  {
    return m_nEntries;
  }
  
  /**
   * Buffer of log messages.
   * \todo Make this a dynamic vector.
   */
  LogEntry_t m_pLog[LOG_ENTRIES];
  int m_nEntries;
  
  /**
   * Temporary buffer which gets filled by calls to operator<<, and flushed by << End.
   */
  LogEntry m_Buffer;
};

#endif
