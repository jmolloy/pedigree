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

#include <BootstrapInfo.h>
#include <Log.h>
#include <panic.h>
#include <machine/Timer.h>
#include <machine/Machine.h>
#include <utilities/utility.h>
#include <processor/Processor.h>

extern BootstrapStruct_t *g_pBootstrapInfo;

Log Log::m_Instance;
BootProgressUpdateFn g_BootProgress = 0;
BootProgressTotalFn g_BootProgressTotal = 0;
//size_t g_BootProgressTotal = 0;
//size_t g_BootProgressCurrent = 0;

void outputSerial(const char* str)
{
    Machine::instance().getSerial(0)->write(str);
}

Log::Log () :
    m_Lock(),
    m_StaticEntries(0),
    m_StaticEntryStart(0),
    m_StaticEntryEnd(0),
    m_Buffer(),
    m_NumberType(Dec),
    #ifdef DONT_LOG_TO_SERIAL
    m_EchoToSerial(false)
    #else
    m_EchoToSerial(true)
    #endif
{
}

Log::~Log ()
{
}

void Log::initialise ()
{
    char *cmdline = g_pBootstrapInfo->getCommandLine();
    if(cmdline)
    {
        List<String*> cmds = String(cmdline).tokenise(' ');
        for (List<String*>::Iterator it = cmds.begin();
        it != cmds.end();
        it++)
        {
            String *cmd = *it;
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
    if(m_EchoToSerial)
        installCallback(outputSerial);
}

void Log::installCallback(OutputCallback callback)
{
    LockGuard<Spinlock> guard(m_Lock);
    OutputCallbackItem *item = new OutputCallbackItem;
    item->func = callback;
    m_OutputCallbacks.pushBack(item);
}
void Log::removeCallback(OutputCallback callback)
{
    LockGuard<Spinlock> guard(m_Lock);
    for(List<OutputCallbackItem*>::Iterator it = m_OutputCallbacks.begin();
        it != m_OutputCallbacks.end();
        it++)
    {
        if(*it && ((*it)->func == callback))
        {
            m_OutputCallbacks.erase(it);
            return;
        }
    }
}

Log &Log::operator<< (const char *str)
{
    m_Buffer.str.append(str);
    return *this;
}

Log &Log::operator<< (String str)
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

// NOTE: Make sure that the templated << operator gets only instantiated for
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
            str += m_Buffer.str;
            str += "\n";

            for(List<OutputCallbackItem*>::Iterator it = m_OutputCallbacks.begin();
                it != m_OutputCallbacks.end();
                it++)
            {
                if(*it)
                    (*it)->func(static_cast<const char*>(str));
            }
        }
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
