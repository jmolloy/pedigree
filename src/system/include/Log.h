/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifdef THREADS
#include <Spinlock.h>
#endif
#include <processor/types.h>
#include <utilities/String.h>
#include <utilities/Vector.h>
#include <utilities/StaticString.h>
#include <panic.h>

/** @addtogroup kernel
 * @{ */

#define SHOW_FILE_IN_LOGS 0

#ifdef THREADS
#define LOG_LOCK_ACQUIRE Log::instance().m_Lock.acquire()
#define LOG_LOCK_RELEASE Log::instance().m_Lock.release()
#else
#define LOG_LOCK_ACQUIRE do { } while(0)
#define LOG_LOCK_RELEASE do { } while(0)
#endif

#if SHOW_FILE_IN_LOGS
#define FILE_LOG(level) \
  do \
  { \
    Log::instance() << level << __FILE__ << ":" << Dec << __LINE__ << Hex << " " << __FUNCTION__ << " -- "; \
  } while(0)
#else
#define FILE_LOG(level)
#endif

#define LOG_AT_LEVEL(level, text, lock) \
  do \
  { \
    if (lock) \
      LOG_LOCK_ACQUIRE; \
    FILE_LOG(level); \
    Log::instance() << level << text << Flush; \
    if (lock) \
      LOG_LOCK_RELEASE; \
  } \
  while (0)

#ifndef NO_LOGGING

/** Add a debug item to the log */
#ifdef DEBUG_LOGGING
#define DEBUG_LOG(text) LOG_AT_LEVEL(Log::Debug, text, 1)
#define DEBUG_LOG_NOLOCK(text) LOG_AT_LEVEL(Log::Debug, text, 0)
#else
#define DEBUG_LOG(text)
#define DEBUG_LOG_NOLOCK(text)
#endif

/** Add a notice to the log */
#define NOTICE(text) LOG_AT_LEVEL(Log::Notice, text, 1)
#define NOTICE_NOLOCK(text) LOG_AT_LEVEL(Log::Notice, text, 0)

/** Add a warning message to the log */
#define WARNING(text) LOG_AT_LEVEL(Log::Warning, text, 1)
#define WARNING_NOLOCK(text) LOG_AT_LEVEL(Log::Warning, text, 0)

/** Add a error message to the log */
#define ERROR(text) LOG_AT_LEVEL(Log::Error, text, 1)
#define ERROR_NOLOCK(text) LOG_AT_LEVEL(Log::Error, text, 0)

/** Add a fatal message to the log
 *  Breaks into debugger and panics if the debugger isn't around, or the user
 *  exits it.
 */
#define FATAL(text) do { LOG_AT_LEVEL(Log::Fatal, text, 1); while(1); } while(0)
#define FATAL_NOLOCK(text) do { LOG_AT_LEVEL(Log::Fatal, text, 0); while(1); } while(0)

#else  // NO_LOGGING

#define DEBUG_LOG(text)
#define DEBUG_LOG_NOLOCK(text)
#define NOTICE(text)
#define NOTICE_NOLOCK(text)
#define WARNING(text)
#define WARNING_NOLOCK(text)
#define ERROR(text)
#define ERROR_NOLOCK(text)
#define FATAL(text)
#define FATAL_NOLOCK(text)

#endif

/** The maximum length of an individual static log entry. */
#define LOG_LENGTH  128
/** The maximum number of static entries in the log. */
#ifdef HUGE_STATIC_LOG
// 2MB static log buffer
#define LOG_ENTRIES ((1<<21)/sizeof(LogEntry))
#else
// 64K static log buffer
#define LOG_ENTRIES ((1<<16)/sizeof(LogEntry))
#endif

/** Radix for Log's integer output */
enum NumberType
{
  /** Hexadecimal */
  Hex,
  /** Decimal */
  Dec,
  /** Octal */
  Oct
};

/** Modifiers for Log */
enum Modifier
{
  /** Flush this log entry */
  Flush
};

// Function pointer to update boot progress -
// Description.
typedef void (*BootProgressUpdateFn)(const char *);

extern size_t g_BootProgressCurrent;
extern size_t g_BootProgressTotal;
extern BootProgressUpdateFn g_BootProgressUpdate;

/** Implements a kernel log that can be used to debug problems.
 *\brief the kernel's log
 *\note You should use the NOTICE, WARNING, ERROR and FATAL macros to write something
 *      into the log. Direct access to the log should only be needed to retrieve
 *      the entries from the log (within the debugger's log viewer for example). */
class Log
{
public:

  /** Output callback function type. Inherit and implement callback to use. */
  class LogCallback
  {
    public:
      virtual void callback(const char *) = 0;

      virtual ~LogCallback() // Virtual destructor for inheritance
      {
      }
  };

  /** Severity level of the log entry */
  enum SeverityLevel
  {
    Debug = 0,
    Notice,
    Warning,
    Error,
    Fatal
  };

  /** The lock
   *\note this should only be acquired by the NOTICE, WARNING, ERROR and FATAL macros */
#ifdef THREADS
  Spinlock m_Lock;
#endif

  /** Retrieves the static Log instance.
   *\return instance of the log class */
  inline static Log &instance()
    {return m_Instance;}

   /** Initialises the Log */
  void initialise1();
  
   /** Initialises the default Log callback (to a serial port) */
  void initialise2();

  /** Installs an output callback */
  void installCallback(LogCallback *pCallback, bool bSkipBacklog=false);

  /** Removes an output callback */
  void removeCallback(LogCallback *pCallback);

  /** Adds an entry to the log.
   *\param[in] str the null-terminated ASCII string that should be added */
  Log &operator<< (const char *str);
  Log &operator<< (String str);
  /** Adds an entry to the log
   *\param[in] str the null-terminated ASCII string that should be added */
  inline Log &operator<< (char *str)
    {return (*this) << (reinterpret_cast<const char*>(str));}
  /** Adds an entry to the log
   *\param[in] b boolean value */
  Log &operator<< (bool b);
  /** Adds an entry to the log
   *\param[in] p pointer value */
  template<class T>
  Log &operator<< (T *p)
    {return (*this) << (reinterpret_cast<uintptr_t>(p));}
  /** Adds an entry to the log (integer type)
   *\param[in] n the number */
  template<class T>
  Log &operator << (T n);

  /** Starts an entry in the log. */
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
    {return 0;}

  /** Stores an entry in the log.
   *\param[in] T type of the log's text */
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
    StaticString<LOG_LENGTH> str;
  };

  /** Type of a static log entry (no memory-management involved) */
  typedef LogEntry StaticLogEntry;
  typedef LogEntry DynamicLogEntry;

  /** Returns the n'th static log entry, counting from the start. */
  inline const StaticLogEntry &getStaticEntry(size_t n) const
    {return m_StaticLog[(m_StaticEntryStart+n) % LOG_ENTRIES];}
  /** Returns the (n - getStaticEntryCount())'th dynamic log entry */
  inline const DynamicLogEntry &getDynamicEntry(size_t n) const
    {return m_StaticLog[0];}

  bool echoToSerial()
    {return m_EchoToSerial;}

  inline const LogEntry &getLatestEntry() const
    {return m_StaticLog[m_StaticEntries - 1];}

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
//  Vector<DynamicLogEntry*> m_DynamicLog;
  /** Number of entries in the static log */
  size_t m_StaticEntries;

  size_t m_StaticEntryStart, m_StaticEntryEnd;

  /** Temporary buffer which gets filled by calls to operator<<, and flushed by << Flush. */
  StaticLogEntry m_Buffer;

  /** The number type mode that we are in. */
  NumberType m_NumberType;

  /** If we should output to serial */
  bool m_EchoToSerial;

  /** Output callback list */
  List<LogCallback*> m_OutputCallbacks;

  /** The Log instance (singleton class) */
  static Log m_Instance;
};

/** @} */

#endif
