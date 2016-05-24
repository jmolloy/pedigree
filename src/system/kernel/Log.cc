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

#include <BootstrapInfo.h>
#include <Log.h>
#include <panic.h>
#include <machine/Timer.h>
#include <machine/Machine.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <LockGuard.h>
#include <time/Time.h>

extern BootstrapStruct_t *g_pBootstrapInfo;

Log Log::m_Instance;
BootProgressUpdateFn g_BootProgressUpdate = 0;
size_t g_BootProgressTotal = 0;
size_t g_BootProgressCurrent = 0;

static NormalStaticString getTimestamp()
{
    Time::Timestamp t = Time::getTime();

    NormalStaticString r;
    r += "[";
    r.append(t);
    r += "] ";
    return r;
}

Log::Log () :
#ifdef THREADS
    m_Lock(),
#endif
    m_StaticEntries(0),
    m_StaticEntryStart(0),
    m_StaticEntryEnd(0),
    m_Buffer(),
#ifdef DONT_LOG_TO_SERIAL
    m_EchoToSerial(false)
#else
    m_EchoToSerial(true)
#endif
{
}

Log::~Log ()
{
    LogEntry entry;
    entry << Notice << "-- Log Terminating --";
    *this << entry << Flush;
}

void Log::initialise1()
{
#ifndef ARM_COMMON
    char *cmdline = g_pBootstrapInfo->getCommandLine();
    if(cmdline)
    {
        List<SharedPointer<String>> cmds = String(cmdline).tokenise(' ');
        for (auto it = cmds.begin(); it != cmds.end(); it++)
        {
            auto cmd = *it;
            if(*cmd == String("--disable-log-to-serial"))
            {
                m_EchoToSerial = false;
                break;
            }
            else if(*cmd == String("--enable-log-to-serial"))
            {
                m_EchoToSerial = true;
                break;
            }
        }
    }
#endif
}

void Log::initialise2()
{
#ifndef DONT_LOG_TO_SERIAL
    if(m_EchoToSerial)
        installSerialLogger();
#endif
}

void Log::installCallback(LogCallback *pCallback, bool bSkipBacklog)
{
    {
#ifdef THREADS
        LockGuard<Spinlock> guard(m_Lock);
#endif
        m_OutputCallbacks.pushBack(pCallback);
    }

    // Some callbacks want to skip a (potentially) massive backlog
    if(bSkipBacklog)
        return;

    // Call the callback for the existing, flushed, log entries
    size_t entry = m_StaticEntryStart;
    while(1)
    {
        if(entry == m_StaticEntryEnd)
            break;
        else
        {
            HugeStaticString str;
            switch(m_StaticLog[entry].type)
            {
                case Debug:
                    str = "(DD) ";
                    break;
                case Notice:
                    str = "(NN) ";
                    break;
                case Warning:
                    str = "(WW) ";
                    break;
                case Error:
                    str = "(EE) ";
                    break;
                case Fatal:
                    str = "(FF) ";
                    break;
            }
            str += getTimestamp();
            str += m_StaticLog[entry].str;
#ifndef SERIAL_IS_FILE
            str += "\r\n"; // Handle carriage return
#else
            str += "\n";
#endif

            /// \note This could send a massive batch of log entries on the
            ///       callback. If the callback isn't designed to handle big
            ///       buffers this may fail.
            pCallback->callback(str);
        }

        entry = (entry + 1) % LOG_ENTRIES;
    }
}
void Log::removeCallback(LogCallback *pCallback)
{
#ifdef THREADS
    LockGuard<Spinlock> guard(m_Lock);
#endif
    for(List<LogCallback*>::Iterator it = m_OutputCallbacks.begin();
        it != m_OutputCallbacks.end();
        it++)
    {
        if(*it == pCallback)
        {
            m_OutputCallbacks.erase(it);
            return;
        }
    }
}

Log::LogEntry &Log::LogEntry::operator<< (const char *s)
{
    str.append(s);
    return *this;
}

Log::LogEntry &Log::LogEntry::operator<< (const String &s)
{
    str.append(s);
    return *this;
}

Log::LogEntry &Log::LogEntry::operator<< (bool b)
{
    if (b)
        return *this << "true";

    return *this << "false";
}

template<class T>
Log::LogEntry &Log::LogEntry::operator << (T n)
{
    size_t radix = 10;
    if (numberType == Hex)
    {
        radix = 16;
        str.append("0x");
    }
    else if (numberType == Oct)
    {
        radix = 8;
        str.append("0");
    }
    str.append(n, radix);
    return *this;
}

Log::LogEntry &Log::LogEntry::operator<< (NumberType type)
{
    numberType = type;
    return *this;
}

Log::LogEntry &Log::LogEntry::operator<< (SeverityLevel level)
{
    // Zero the buffer.
    str.clear();
    type = level;

#ifndef UTILITY_LINUX
    Machine &machine = Machine::instance();
    if (machine.isInitialised() == true &&
        machine.getTimer() != 0)
    {
        Timer &timer = *machine.getTimer();
        timestamp = timer.getTickCount();
    }
    else
        timestamp = 0;
#endif

    return *this;
}

// NOTE: Make sure that the templated << operator gets only instantiated for
//       integer types.
template Log::LogEntry &Log::LogEntry::operator << (char);
template Log::LogEntry &Log::LogEntry::operator << (unsigned char);
template Log::LogEntry &Log::LogEntry::operator << (short);
template Log::LogEntry &Log::LogEntry::operator << (unsigned short);
template Log::LogEntry &Log::LogEntry::operator << (int);
template Log::LogEntry &Log::LogEntry::operator << (unsigned int);
template Log::LogEntry &Log::LogEntry::operator << (long);
template Log::LogEntry &Log::LogEntry::operator << (unsigned long);
// NOTE: Instantiating these for MIPS32 requires __udiv3di, but we only have
//       __udiv3ti (??) in libgcc.a for mips.
#ifndef MIPS32
template Log::LogEntry &Log::LogEntry::operator << (long long);
template Log::LogEntry &Log::LogEntry::operator << (unsigned long long);
#endif

Log &Log::operator<< (const LogEntry &entry)
{
    m_Buffer = entry;
    return *this;
}

Log &Log::operator<< (Modifier type)
{
    static bool handlingFatal = false;

    // Flush the buffer.
    if (type == Flush)
    {
        if (m_StaticEntries >= LOG_ENTRIES)
        {
            m_StaticEntryStart = (m_StaticEntryStart+1) % LOG_ENTRIES;
        }
        else
            m_StaticEntries ++;

        m_StaticLog[m_StaticEntryEnd] = m_Buffer;
        m_StaticEntryEnd = (m_StaticEntryEnd+1) % LOG_ENTRIES;

        if(m_OutputCallbacks.count())
        {
            // We have output callbacks installed. Build the string we'll pass
            // to each callback *now* and then send it.
            HugeStaticString str;
            switch(m_Buffer.type)
            {
                case Debug:
                    str = "(DD) ";
                    break;
                case Notice:
                    str = "(NN) ";
                    break;
                case Warning:
                    str = "(WW) ";
                    break;
                case Error:
                    str = "(EE) ";
                    break;
                case Fatal:
                    str = "(FF) ";
                    break;
            }
            str += getTimestamp();
            str += m_Buffer.str;
#ifndef SERIAL_IS_FILE
            str += "\r\n"; // Handle carriage return
#else
            str += "\n";
#endif

            for(List<LogCallback*>::Iterator it = m_OutputCallbacks.begin();
                it != m_OutputCallbacks.end();
                ++it)
            {
                if(*it)
                    (*it)->callback(static_cast<const char*>(str));
            }
        }

        // Panic if that was a fatal error.
        if ((!handlingFatal) && m_Buffer.type == Fatal)
        {
            handlingFatal = true;

            const char *panicstr = static_cast<const char*>(m_Buffer.str);
#ifdef THREADS
            if (m_Lock.acquired())
                m_Lock.release();
#endif

            // Attempt to trap to debugger, panic if that fails.
#ifdef DEBUGGER
            Processor::breakpoint();
#endif
            panic(panicstr);
        }
    }

    return *this;
}

Log::LogCallback::~LogCallback()
{
}
