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

#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include <processor/types.h>
#include <utilities/String.h>
#include <utilities/Vector.h>
#include <utilities/StaticString.h>

/** @addtogroup kernel
 * @{ */

/** Add a notice to the log */
#define NOTICE(text) \
  do \
  { \
    Log::instance() << Log::Notice << text << Flush; \
  } \
  while (0)

/** Add a warning message to the log */
#define WARNING(text) \
  do \
  { \
    Log::instance() << Log::Warning << text << Flush; \
  } \
  while (0)

/** Add a error message to the log */
#define ERROR(text) \
  do \
  { \
    Log::instance() << Log::Error << text << Flush; \
  } \
  while (0)

/** Add a fatal message to the log */
#define FATAL(text) \
  do \
  { \
    Log::instance() << Log::Fatal << text << Flush; \
  } \
  while (0)

/** The maximum length of an individual static log entry. */
#define LOG_LENGTH  128
/** The maximum number of static entries in the log. */
#define LOG_ENTRIES 32

/** Radix for Log's integer output */
enum NumberType
{
  /** Hexadecimal */
  Hex,
  /** Decimal */
  Dec
};

/** Modifiers for Log */
enum Modifier
{
  /** Flush this log entry */
  Flush
};

/** Implements a kernel log that can be used to debug problems.
 *\brief the kernel's log
 *\note You should use the NOTICE, WARNING, ERROR and FATAL macros to write something
 *      into the log. Direct access to the log should only be needed to rethrieve
 *      the entries from the log (within the debugger's log viewer for example). */
class Log
{
public:
  /** Severity level of the log entry */
  enum SeverityLevel
  {
    Notice,
    Warning,
    Error,
    Fatal
  };

  /** Retrieves the static Log instance.
   *\return instance of the log class */
  inline static Log &instance()
    {return m_Instance;}

  /** Adds an entry to the log.
   *\param[in] str the null-terminated ASCII string that should be added */
  Log &operator<< (const char *str);
  /** Adds an entry to the log
   *\param[in] str the null-terminated ASCII string that should be added */
  inline Log &operator<< (char *str)
    {return (*this) << (reinterpret_cast<const char*>(str));}
  /** Adds an entry to the log
   *\param[in] b boolean value */
  Log &operator<< (bool b);
  /** Adds an entry to the log (integer type)
   *\param[in] n the number */
  template<class T>
  Log &operator << (T n);

  /** Starts an entry in the log.
   *\todo This function should gain and release spinlocks, depending on level. */
  Log &operator<< (SeverityLevel level);
  /** Changes the number type between hex and decimal. */
  Log &operator<< (NumberType type);
  /** Modifier */
  Log &operator<< (Modifier type);

  /** Get the number of static entries in the log.
   *\return the number of static entries in the log */
  inline size_t getStaticEntryCount() const
    {return m_StaticEntries;}
  /** Get the number of dynamic entries in the log
   *\return the number of dynamic entries in the log */
  inline size_t getDynamicEntryCount() const
    {return m_DynamicLog.count();}

  /** Stores an entry in the log.
   *\param[in] T type of the log's text */
  template<class T>
  struct LogEntry
  {
    /** Constructor does nothing */
    inline LogEntry()
     : timestamp(), type(), str(){}

    /** The time (since boot) that this log entry was added, in ticks. */
    unsigned int timestamp;
    /** The severity level of this entry. */
    SeverityLevel type;
    /** The actual entry text. */
    T str;   
  };

  /** Type of a static log entry (no memory-management involved) */
  typedef LogEntry<StaticString<LOG_LENGTH> > StaticLogEntry;
  /** Type of a dynamic log entry (memory-management involved) */
  typedef LogEntry<String> DynamicLogEntry;

  /** Returns the n'th static log entry, counting from the start. */
  inline const StaticLogEntry &getStaticEntry(size_t n) const
    {return m_StaticLog[n];}
  /** Returns the (n - getStaticEntryCount())'th dynamic log entry */
  inline const DynamicLogEntry &getDynamicEntry(size_t n) const
    {return *m_DynamicLog[n - m_StaticEntries];}

private:
  /** Default constructor - does nothing. */
  Log();
  /** Default destructor - does nothing */
  ~Log();
  /** Copy-constructor
   *\note NOT implemented */
  Log(const Log &);
  /** Assignment operator
   *\note NOT implemented */
  Log &operator = (const Log &);

  /** Static buffer of log messages. */
  StaticLogEntry m_StaticLog[LOG_ENTRIES];
  /** Dynamic buffer of log messages */
  Vector<DynamicLogEntry*> m_DynamicLog;
  /** Number of entries in the static log */
  size_t m_StaticEntries;

  /** Temporary buffer which gets filled by calls to operator<<, and flushed by << Flush. */
  StaticLogEntry m_Buffer;

  /** The number type mode that we are in. */
  NumberType m_NumberType;

  /** The Log instance (singleton class) */
  static Log m_Instance;
};

/** @} */

#endif
