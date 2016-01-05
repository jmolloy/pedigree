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

#include "Console.h"
#include <vfs/VFS.h>
#include <Module.h>

#include <process/Scheduler.h>

// c_cc array indices, and their default character.
#define VEOF        4 // 0x04
#define VEOL        5 // 0x00
#define VERASE      2 // 0x08
#define VINTR       0 // 0x03
#define VKILL       3 // 0x15
#define VQUIT       1 // 0x1C
#define VSTART      11 // 0x11
#define VSTOP       12 // 0x13
#define VSUSP       10 // 0x1A

static const char defaultControl[MAX_CONTROL_CHAR] = {
    0x03, // VINTR
    0x1C, // VQUIT
    0x08, // VERASE
    0x15, // VKILL
    0x04, // VEOF
    0x00, // VEOL
    0x00, // 6
    0x00, // 7
    0x00, // 8
    0x00, // 9
    0x1A, // VSUSP
    0x11, // VSTART
    0x13, // VSTOP
    0x00, // 13
    0x17, // VWERASE
    0x16, // VLNEXT
    0x00, // VEOL2
};

ConsoleManager ConsoleManager::m_Instance;

ConsoleFile::ConsoleFile(String consoleName, Filesystem *pFs) :
    File(consoleName, 0, 0, 0, 0xdeadbeef, pFs, 0, 0),
    m_Flags(DEFAULT_FLAGS), m_Rows(25), m_Cols(80),
    m_RingBuffer(PTY_BUFFER_SIZE), m_Name(consoleName), m_pEvent(0)
{
    memcpy(m_ControlChars, defaultControl, MAX_CONTROL_CHAR);
}

int ConsoleFile::select(bool bWriting, int timeout)
{
    if(timeout)
    {
        while(!m_RingBuffer.waitFor(bWriting ? RingBufferWait::Writing : RingBufferWait::Reading));
        return 1;
    }
    else
    {
        if(bWriting)
            return m_RingBuffer.canWrite();
        else
            return m_RingBuffer.dataReady();
    }
}

ConsoleMasterFile::ConsoleMasterFile(String consoleName, Filesystem *pFs) :
    ConsoleFile(consoleName, pFs), bLocked(false), pLocker(0), m_LineBuffer(), m_LineBufferSize(0),
    m_LineBufferFirstNewline(~0), m_Last(0), m_EventTrigger(true)
{
}

uint64_t ConsoleMasterFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if(bCanBlock)
    {
        if(!m_RingBuffer.waitFor(RingBufferWait::Reading))
            return 0; // Interrupted.
    }
    else if(!m_RingBuffer.dataReady())
    {
        return 0;
    }

    size_t maxSize = size;

    uintptr_t originalBuffer = buffer;
    while(m_RingBuffer.dataReady() && size)
    {
        *reinterpret_cast<char*>(buffer) = m_RingBuffer.read();
        ++buffer;
        --size;
    }

    size_t endSize = outputLineDiscipline(reinterpret_cast<char *>(originalBuffer), buffer - originalBuffer, maxSize);

    return endSize;
}

uint64_t ConsoleMasterFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if(bCanBlock)
    {
        if(!m_pOther->m_RingBuffer.waitFor(RingBufferWait::Writing))
            return 0; // Interrupted.
    }
    else if(!m_pOther->m_RingBuffer.canWrite())
    {
        return 0;
    }

    // Pass on to the input discipline, which will write to the slave.
    inputLineDiscipline(reinterpret_cast<char *>(buffer), size);

    return size;
}

void ConsoleMasterFile::inputLineDiscipline(char *buf, size_t len)
{
    // Make sure we always have the latest flags from the slave.
    size_t slaveFlags = m_pOther->m_Flags;
    const char *slaveControlChars = m_pOther->m_ControlChars;

    size_t localWritten = 0;

    // Handle temios local modes
    if(slaveFlags & (ConsoleManager::LCookedMode|ConsoleManager::LEcho))
    {
        // Whether or not the application buffer has already been filled
        bool bAppBufferComplete = false;

        // Used for raw mode - just a buffer for erase echo etc
        char *destBuff = new char[len];
        size_t destBuffOffset = 0;

        // Iterate over the buffer
        while(!bAppBufferComplete)
        {
            for(size_t i = 0; i < len; i++)
            {
                // Handle incoming newline
                bool isCanonical = (slaveFlags & ConsoleManager::LCookedMode);
                if(isCanonical && (buf[i] == slaveControlChars[VEOF]))
                {
                    // EOF. Write it and it alone to the slave.
                    m_pOther->inject(&buf[i], 1);
                    return;
                }

                if((buf[i] == '\r') || (isCanonical && (buf[i] == slaveControlChars[VEOL])))
                {
                    // LEcho - output the newline. LCookedMode - handle line buffer.
                    if((slaveFlags & ConsoleManager::LEcho) || (slaveFlags & ConsoleManager::LCookedMode))
                    {
                        // Only echo the newline if we are supposed to
                        m_LineBuffer[m_LineBufferSize++] = '\n';
                        if((slaveFlags & ConsoleManager::LEchoNewline) || (slaveFlags & ConsoleManager::LEcho))
                        {
                            m_RingBuffer.write('\n');
                            ++localWritten;
                        }

                        if((slaveFlags & ConsoleManager::LCookedMode) && !bAppBufferComplete)
                        {
                            // Transmit full buffer to slave.
                            size_t realSize = m_LineBufferSize;
                            if(m_LineBufferFirstNewline < realSize)
                            {
                                realSize = m_LineBufferFirstNewline;
                                m_LineBufferFirstNewline = ~0UL;
                            }

                            m_pOther->inject(m_LineBuffer, realSize);

                            // And now move the buffer over the space we just consumed
                            uint64_t nConsumedBytes = m_LineBufferSize - realSize;
                            if(nConsumedBytes) // If zero, the buffer was consumed completely
                                memcpy(m_LineBuffer, &m_LineBuffer[realSize], nConsumedBytes);

                            // Reduce the buffer size now
                            m_LineBufferSize -= realSize;

                            // The buffer has been filled!
                            bAppBufferComplete = true;
                        }
                        else if((slaveFlags & ConsoleManager::LCookedMode) && (m_LineBufferFirstNewline == ~0UL))
                        {
                            // Application buffer has already been filled, let future runs know where the limit is
                            m_LineBufferFirstNewline = m_LineBufferSize - 1;
                        }
                        else if(!(slaveFlags & ConsoleManager::LCookedMode))
                        {
                            // Inject this byte into the slave...
                            destBuff[destBuffOffset++] = buf[i];
                        }

                        // Ignore the \n if one is present
                        if(buf[i+1] == '\n')
                            i++;
                    }
                }
                else if(buf[i] == m_ControlChars[VERASE])
                {
                    if(slaveFlags & (ConsoleManager::LCookedMode|ConsoleManager::LEchoErase))
                    {
                        if((slaveFlags & ConsoleManager::LCookedMode) && m_LineBufferSize)
                        {
                            char ctl[3] = {'\x08', ' ', '\x08'};
                            m_RingBuffer.write(ctl, 3);
                            m_LineBufferSize--;
                            ++localWritten;
                        }
                        else if((!(slaveFlags & ConsoleManager::LCookedMode)) && destBuffOffset)
                        {
                            char ctl[3] = {'\x08', ' ', '\x08'};
                            m_RingBuffer.write(ctl, 3);
                            destBuffOffset--;
                            ++localWritten;
                        }
                    }
                }
                else
                {
                    // Do we need to handle this character differently?
                    if(checkForEvent(slaveFlags, buf[i]))
                    {
                        // So, normally we'll be fine to print nicely, but if
                        // we can't write to the ring buffer, we must not try
                        // to do so. This event may be necessary to unblock the
                        // buffer!
                        if (!m_RingBuffer.canWrite())
                        {
                            // Forcefully clear out bytes so we can write what
                            // we need to to the ring buffer.
                            char buf[3];
                            m_RingBuffer.read(buf, 3);
                        }

                        // Write it to the master nicely (eg, ^C, ^D)
                        char ctl_c = '@' + buf[i];
                        char ctl[3] = {'^', ctl_c, '\n'};
                        m_RingBuffer.write(ctl, 3);
                        ++localWritten;

                        // Trigger the actual event.
                        triggerEvent(buf[i]);
                        continue;
                    }

                    // Write the character to the slave
                    if(slaveFlags & ConsoleManager::LEcho)
                    {
                        m_RingBuffer.write(buf[i]);
                        ++localWritten;
                    }

                    // Add to the buffer
                    if(slaveFlags & ConsoleManager::LCookedMode)
                        m_LineBuffer[m_LineBufferSize++] = buf[i];
                    else
                    {
                        destBuff[destBuffOffset++] = buf[i];
                    }
                }
            }

            // We appear to have hit the top of the line buffer!
            if(m_LineBufferSize >= LINEBUFFER_MAXIMUM)
            {
                // Our best bet is to return early, giving the application what we can of the line buffer
                size_t numBytesToRemove = m_LineBufferSize;

                // Copy the buffer across
                m_pOther->inject(m_LineBuffer, numBytesToRemove);

                // And now move the buffer over the space we just consumed
                uint64_t nConsumedBytes = m_LineBufferSize - numBytesToRemove;
                if(nConsumedBytes) // If zero, the buffer was consumed completely
                    memcpy(m_LineBuffer, &m_LineBuffer[numBytesToRemove], nConsumedBytes);

                // Reduce the buffer size now
                m_LineBufferSize -= numBytesToRemove;
            }

            /// \todo remove me, this is because of the port
            break;
        }

        if(destBuffOffset)
        {
            m_pOther->inject(destBuff, len);
        }

        delete [] destBuff;
    }
    else
    {
        for(size_t i = 0; i < len; ++i)
        {
            // Do we need to send an event?
            if(checkForEvent(slaveFlags, buf[i]))
            {
                triggerEvent(buf[i]);
                continue;
            }

            // No event. Simply write the character out.
            m_pOther->inject(&buf[i], 1);
        }
    }

    // Wake up anything waiting on data to read from us.
    if(localWritten)
        dataChanged();
}

size_t ConsoleMasterFile::outputLineDiscipline(char *buf, size_t len, size_t maxSz)
{
    // Make sure we always have the latest flags from the slave.
    size_t slaveFlags = m_pOther->m_Flags;

    // Post-process output if enabled.
    if(slaveFlags & (ConsoleManager::OPostProcess))
    {
        char *tmpBuff = new char[len];
        size_t realSize = len;

        char *pC = buf;
        for (size_t i = 0, j = 0; j < len; j++)
        {
            bool bInsert = true;

            // OCRNL: Map CR to NL on output
            if (pC[j] == '\r' && (slaveFlags & ConsoleManager::OMapCRToNL))
            {
                tmpBuff[i++] = '\n';
                continue;
            }

            // ONLCR: Map NL to CR-NL on output
            else if (pC[j] == '\n' && (slaveFlags & ConsoleManager::OMapNLToCRNL))
            {
                if (realSize >= maxSz)
                {
                    // We do not have any room to add in the mapped character. Drop it.
                    // WARNING("Console ignored an NL -> CRNL conversion due to a full buffer.");
                    tmpBuff[i++] = '\n';
                    continue;
                }

                realSize++;
                char *newBuff = new char[realSize];
                memcpy(newBuff, tmpBuff, i);
                delete [] tmpBuff;
                tmpBuff = newBuff;

                // Add the newline and the caused carriage return
                tmpBuff[i++] = '\r';
                tmpBuff[i++] = '\n';

                continue;
            }

            // ONLRET: NL performs CR function
            if(pC[j] == '\n' && (slaveFlags & ConsoleManager::ONLCausesCR))
            {
                tmpBuff[i++] = '\r';
                continue;
            }

            if(bInsert)
            {
                tmpBuff[i++] = pC[j];
            }
        }

        memcpy(buf, tmpBuff, realSize);
        delete [] tmpBuff;
        len = realSize;
    }

    return len;
}

bool ConsoleMasterFile::checkForEvent(size_t flags, char check)
{
    const char *slaveControlChars = m_pOther->m_ControlChars;

    // ISIG?
    if(flags & ConsoleManager::LGenerateEvent)
    {
        if(check && (
                    check == slaveControlChars[VINTR] ||
                    check == slaveControlChars[VQUIT] ||
                    check == slaveControlChars[VSUSP]))
        {
            return true;
        }
    }
    return false;
}

void ConsoleMasterFile::triggerEvent(char cause)
{
    if(m_pOther->m_pEvent)
    {
        Thread *pThread = Processor::information().getCurrentThread();
        m_Last = cause;
        pThread->sendEvent(m_pOther->m_pEvent);
        Scheduler::instance().yield();

        // Note that we do not release the mutex here.
        while(!m_EventTrigger.acquire());
    }
}

ConsoleSlaveFile::ConsoleSlaveFile(String consoleName, Filesystem *pFs) :
    ConsoleFile(consoleName, pFs)
{
}

uint64_t ConsoleSlaveFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if(bCanBlock)
    {
        if(!m_RingBuffer.waitFor(RingBufferWait::Reading))
            return 0; // Interrupted
    }
    else if(!m_RingBuffer.dataReady())
    {
        return 0;
    }

    uintptr_t originalBuffer = buffer;
    while(m_RingBuffer.dataReady() && size)
    {
        *reinterpret_cast<char*>(buffer) = m_RingBuffer.read();
        ++buffer;
        --size;
    }

    size_t endSize = processInput(reinterpret_cast<char *>(originalBuffer), buffer - originalBuffer);

    return endSize;
}

uint64_t ConsoleSlaveFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if(bCanBlock)
    {
        if(!m_pOther->m_RingBuffer.waitFor(RingBufferWait::Writing))
            return 0; // Interrupted.
    }
    else if(!m_pOther->m_RingBuffer.canWrite())
    {
        return 0;
    }

    // Send straight to the master.
    m_pOther->inject(reinterpret_cast<char *>(buffer), size);

    return size;
}

size_t ConsoleSlaveFile::processInput(char *buf, size_t len)
{
    // Perform input processing.
    char *pC = buf;
    size_t realLen = len;
    for (size_t i = 0; i < len; i++)
    {
        if (m_Flags & ConsoleManager::IStripToSevenBits)
            pC[i] = static_cast<uint8_t>(pC[i]) & 0x7F;
        if (m_Flags & ConsoleManager::LCookedMode)
        {
            if (pC[i] == m_ControlChars[VEOF])
            {
                // Zero-length read: EOF.
                realLen = 0;
                break;
            }
        }

        if (pC[i] == '\n' && (m_Flags & ConsoleManager::IMapNLToCR))
            pC[i] = '\r';
        else if (pC[i] == '\r' && (m_Flags & ConsoleManager::IMapCRToNL))
            pC[i] = '\n';
        else if (pC[i] == '\r' && (m_Flags & ConsoleManager::IIgnoreCR))
        {
            memmove(buf+i, buf+i+1, len-i-1);
            i--; // Need to process this byte again, its contents have changed.
            realLen--;
        }
    }

    return realLen;
}

void ConsoleManager::newConsole(char c, size_t i)
{
    char a = 'a' + (i % 10);
    if (i <= 9)
        a = '0' + i;

    char master[] = {'p', 't', 'y', c, a, 0};
    char slave[] = {'t', 't', 'y', c, a, 0};

    String masterName(master), slaveName(slave);

    ConsoleMasterFile *pMaster = new ConsoleMasterFile(masterName, this);
    ConsoleSlaveFile *pSlave = new ConsoleSlaveFile(slaveName, this);

    pMaster->setOther(pSlave);
    pSlave->setOther(pMaster);

    {
        LockGuard<Spinlock> guard(m_Lock);
        m_Consoles.pushBack(pMaster);
        m_Consoles.pushBack(pSlave);
    }
}

ConsoleManager::ConsoleManager() :
    m_Consoles(), m_Lock()
{
    // Create all consoles, so we can look them up easily.
    for(size_t i = 0; i < 16; ++i)
    {
        for(char c = 'p'; c <= 'z'; ++c)
        {
            newConsole(c, i);
        }
        for(char c = 'a'; c <= 'e'; ++c)
        {
            newConsole(c, i);
        }
    }
}

ConsoleManager::~ConsoleManager()
{
}

ConsoleManager &ConsoleManager::instance()
{
    return m_Instance;
}

File* ConsoleManager::getConsole(String consoleName)
{
    LockGuard<Spinlock> guard(m_Lock);
    for (size_t i = 0; i < m_Consoles.count(); i++)
    {
        ConsoleFile *pC = m_Consoles[i];
        if (pC->m_Name == consoleName)
        {
            return pC;
        }
    }
    // Error - not found.
    return 0;
}

ConsoleFile *ConsoleManager::getConsoleFile(RequestQueue *pBackend)
{
    return 0;
}

bool ConsoleManager::lockConsole(File *file)
{
    if(!isConsole(file))
        return false;

    ConsoleMasterFile *pConsole = static_cast<ConsoleMasterFile *>(file);
    if(!pConsole->isMaster())
        return false;

    if(pConsole->bLocked)
        return false;

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    pConsole->bLocked = true;
    pConsole->pLocker = pProcess;

    return true;
}

void ConsoleManager::unlockConsole(File *file)
{
    if(!isConsole(file))
        return;

    ConsoleMasterFile *pConsole = static_cast<ConsoleMasterFile *>(file);
    if(!pConsole->isMaster())
        return;

    // Make sure we are the owner of the master.
    // Forked children shouldn't be able to close() and steal a master pty.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    if(pConsole->pLocker != pProcess)
        return;
    pConsole->bLocked = false;
}

bool ConsoleManager::isConsole(File* file)
{
    if(!file)
        return false;
    return (file->getInode() == 0xdeadbeef);
}

bool ConsoleManager::isMasterConsole(File *file)
{
    if(!isConsole(file))
        return false;

    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    return pFile->isMaster();
}

void ConsoleManager::setAttributes(File* file, size_t flags)
{
    // \todo Sanity checking of the flags.
    if(!file)
        return;
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    pFile->m_Flags = flags;
}

void ConsoleManager::getAttributes(File* file, size_t *flags)
{
    if(!file || !flags)
        return;
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    *flags = pFile->m_Flags;
}

void ConsoleManager::setControlChars(File *file, void *p)
{
    if(!file || !p)
        return;
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    memcpy(pFile->m_ControlChars, p, MAX_CONTROL_CHAR);
}

void ConsoleManager::getControlChars(File *file, void *p)
{
    if(!file || !p)
        return;
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    memcpy(p, pFile->m_ControlChars, MAX_CONTROL_CHAR);
}

int ConsoleManager::getWindowSize(File *file, unsigned short *rows, unsigned short *cols)
{
    if(!file)
        return -1;
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    if(!pFile->isMaster())
        pFile = pFile->m_pOther;

    *rows = pFile->m_Rows;
    *cols = pFile->m_Cols;
    return 0;
}

int ConsoleManager::setWindowSize(File *file, unsigned short rows, unsigned short cols)
{
    if(!file)
        return false;
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    if(!pFile->isMaster())
    {
        // Ignore. Slave cannot change window size.
        return 0;
    }
    pFile->m_Rows = rows;
    pFile->m_Cols = cols;
    return 0;
}

bool ConsoleManager::hasDataAvailable(File* file)
{
    if(!file)
        return false;
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    return pFile->select(false, 0);
}

void ConsoleManager::flush(File *file)
{
}

File *ConsoleManager::getOther(File *file)
{
    if(!file)
        return 0;
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    return pFile->m_pOther;
}

static bool initConsole()
{
    return true;
}

static void destroyConsole()
{
}

MODULE_INFO("console", &initConsole, &destroyConsole, "vfs");
