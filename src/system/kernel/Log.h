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

#include <processor/types.h>

#define NOTICE(text) \
  do \
  { \
    Log::instance() << Log::Notice << text << Flush; \
  } \
  while (0)

#define WARNING(text) \
  do \
  { \
    Log::instance() << Log::Warning << text << Flush; \
  } \
  while (0)

#define ERROR(text) \
  do \
  { \
    Log::instance() << Log::Error << text << Flush; \
  } \
  while (0)

#define FATAL(text) \
  do \
  { \
    Log::instance() << Log::Fatal << text << Flush; \
  } \
  while (0)

/// The maximum length of an individual log entry.
/// \todo Change to using dynamic memory.
#define LOG_LENGTH  128
/// The maximum number of entries in the log.
/// \todo Change to using dynamic memory.
#define LOG_ENTRIES 64

enum NumberType
{
  Hex,
  Dec
};

enum Modifier
{
  Flush
};

/**
 * Implements a kernel log that can be used to debug problems.
 *\todo implementation might want to use static string
 *\todo << operator for all integer data types
 */
class Log
{
public:
  enum SeverityLevel
  {
    Notice,
    Warning,
    Error,
    Fatal
  };
  
  /**
   * Stores an entry in the log.
   */
  typedef struct LogEntry
  {
    unsigned int timestamp; ///< The time (since boot) that this log entry was added, in ticks.
    SeverityLevel type;     ///< The severity level of this entry.
    char str[LOG_LENGTH];   ///< The actual entry text. \todo Change this to using dynamic memory. Might be problematic, because we might want to use the log (and display it) before memory-managment is initialised.
  } LogEntry_t;

  /**
   * Retrieves the static Log instance.
   */
  static Log &instance()
  {
    return m_Instance;
  }
  
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

  Log &operator<< (Modifier type);

  /**
   * Returns the n'th log entry, counting from the start.
   */
  LogEntry_t getEntry(size_t n)
  {
    return m_pLog[n];
  }
  
  /**
   * Returns the number of entries in the log.
   */
  size_t getEntryCount()
  {
    return m_nEntries;
  }

private:
  /**
   * Default constructor - does nothing.
   */
  Log ();
  ~Log ();
  
  /**
   * Prints a hex number to the log.
   */
  void writeHex(unsigned int n);
  
  /**
   * Prints a decimal number to the log.
   */
  void writeDec(unsigned int n);
  
  /**
   * Buffer of log messages.
   * \todo Make this a dynamic vector.
   */
  LogEntry_t m_pLog[LOG_ENTRIES];
  size_t m_nEntries;
  
  /**
   * Temporary buffer which gets filled by calls to operator<<, and flushed by << End.
   */
  LogEntry m_Buffer;
  
  /**
   * The number type mode that we are in.
   */
  NumberType m_NumberType;
  
  /**
   * The Log instance (singleton class)
   */
  static Log m_Instance;
};

#endif
