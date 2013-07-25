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

#include "Console.h"
#include <vfs/VFS.h>
#include <Module.h>

#include <process/Scheduler.h>

ConsoleManager ConsoleManager::m_Instance;

ConsoleFile::ConsoleFile(String consoleName, Filesystem *pFs) :
    File(consoleName, 0, 0, 0, 0xdeadbeef, pFs, 0, 0),
    m_Flags(DEFAULT_FLAGS), m_Name(), m_RingBuffer(PTY_BUFFER_SIZE)
{
}

int ConsoleFile::select(bool bWriting, int timeout)
{
    if(timeout)
    {
        m_RingBuffer.waitFor(bWriting ? RingBufferWait::Writing : RingBufferWait::Reading);
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
    ConsoleFile(consoleName, pFs), bLocked(false), m_LineBuffer(), m_LineBufferSize(0),
    m_LineBufferFirstNewline(~0)
{
}

uint64_t ConsoleMasterFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if(bCanBlock)
    {
        m_RingBuffer.waitFor(RingBufferWait::Reading);
    }
    else if(!m_RingBuffer.dataReady())
    {
        return 0;
    }

    uintptr_t originalBuffer = location;
    while(m_RingBuffer.dataReady() && size)
    {
        *reinterpret_cast<char*>(location) = m_RingBuffer.read();
        ++location;
        --size;
    }

    size_t endSize = outputLineDiscipline(reinterpret_cast<char *>(originalBuffer), location - originalBuffer);

    return endSize;
}

uint64_t ConsoleMasterFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if(bCanBlock)
    {
        m_RingBuffer.waitFor(RingBufferWait::Writing);
    }
    else if(!m_RingBuffer.canWrite())
    {
        return 0;
    }

    uintptr_t originalBuffer = location;
    while(m_RingBuffer.canWrite() && size)
    {
        m_RingBuffer.write(*reinterpret_cast<char*>(location));
        ++location;
        --size;
    }

    inputLineDiscipline(reinterpret_cast<char *>(originalBuffer), location - originalBuffer);

    return location - originalBuffer;
}

void ConsoleMasterFile::inputLineDiscipline(char *buf, size_t len)
{
    // Handle temios local modes
    if(m_Flags & (ConsoleManager::LCookedMode|ConsoleManager::LEcho))
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
                if(buf[i] == '\r')
                {
                    // LEcho - output the newline. LCookedMode - handle line buffer.
                    if((m_Flags & ConsoleManager::LEcho) || (m_Flags & ConsoleManager::LCookedMode))
                    {
                        // Only echo the newline if we are supposed to
                        m_LineBuffer[m_LineBufferSize++] = '\n';
                        if((m_Flags & ConsoleManager::LEchoNewline) || (m_Flags & ConsoleManager::LEcho))
                            m_RingBuffer.write('\n');

                        if((m_Flags & ConsoleManager::LCookedMode) && !bAppBufferComplete)
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
                        else if((m_Flags & ConsoleManager::LCookedMode) && (m_LineBufferFirstNewline == ~0UL))
                        {
                            // Application buffer has already been filled, let future runs know where the limit is
                            m_LineBufferFirstNewline = m_LineBufferSize - 1;
                        }
                        else if(!(m_Flags & ConsoleManager::LCookedMode))
                        {
                            // Inject this byte into the slave...
                            destBuff[destBuffOffset++] = buf[i];
                        }

                        // Ignore the \n if one is present
                        if(buf[i+1] == '\n')
                            i++;
                    }
                }
                else if(buf[i] == '\x08')
                {
                    if(m_Flags & (ConsoleManager::LCookedMode|ConsoleManager::LEchoErase))
                    {
                        if((m_Flags & ConsoleManager::LCookedMode) && m_LineBufferSize)
                        {
                            char ctl[3] = {'\x08', ' ', '\x08'};
                            m_RingBuffer.write(ctl, 3);
                            m_LineBufferSize--;
                        }
                        else if((!(m_Flags & ConsoleManager::LCookedMode)) && destBuffOffset)
                        {
                            char ctl[3] = {'\x08', ' ', '\x08'};
                            m_RingBuffer.write(ctl, 3);
                            destBuffOffset--;
                        }
                    }
                }
                else if(buf[i] < 0x1F)
                {
                    /*
                    if(readBuffer[i] == 0x3)
                    {
                        Process *p = Processor::information().getCurrentThread()->getParent();
                        p->getSubsystem()->kill(Subsystem::Interrupted);
                    }
                    */
                }
                else
                {
                    // Write the character to the console
                    /// \todo ISIG handling, special characters
                    if(m_Flags & ConsoleManager::LEcho)
                        m_RingBuffer.write(buf[i]);

                    // Add to the buffer
                    if(m_Flags & ConsoleManager::LCookedMode)
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
    }
    else
    {
        m_pOther->inject(buf, len);
    }
}

size_t ConsoleMasterFile::outputLineDiscipline(char *buf, size_t len)
{
    // Post-process output if enabled.
    if(m_Flags & (ConsoleManager::OPostProcess))
    {
        char *tmpBuff = new char[len];
        size_t realSize = len;

        char *pC = buf;
        for (size_t i = 0, j = 0; j < len; j++)
        {
            bool bInsert = true;

            // OCRNL: Map CR to NL on output
            if (pC[j] == '\r' && (m_Flags & ConsoleManager::OMapCRToNL))
            {
                tmpBuff[i++] = '\n';
                continue;
            }

            // ONLCR: Map NL to CR-NL on output
            else if (pC[j] == '\n' && (m_Flags & ConsoleManager::OMapNLToCRNL))
            {
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
            if(pC[j] == '\n' && (m_Flags & ConsoleManager::ONLCausesCR))
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

ConsoleSlaveFile::ConsoleSlaveFile(String consoleName, Filesystem *pFs) :
    ConsoleFile(consoleName, pFs)
{
}

uint64_t ConsoleSlaveFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if(bCanBlock)
    {
        m_RingBuffer.waitFor(RingBufferWait::Reading);
    }
    else if(!m_RingBuffer.dataReady())
    {
        return 0;
    }

    uintptr_t originalBuffer = location;
    while(m_RingBuffer.dataReady() && size)
    {
        *reinterpret_cast<char*>(location) = m_RingBuffer.read();
        ++location;
        --size;
    }

    size_t endSize = processInput(reinterpret_cast<char *>(originalBuffer), location - originalBuffer);

    return endSize;
}

uint64_t ConsoleSlaveFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if(bCanBlock)
    {
        m_RingBuffer.waitFor(RingBufferWait::Writing);
    }
    else if(!m_RingBuffer.canWrite())
    {
        return 0;
    }

    uintptr_t originalBuffer = location;
    while(m_RingBuffer.canWrite() && size)
    {
        m_RingBuffer.write(*reinterpret_cast<char*>(location));
        ++location;
        --size;
    }

    return location - originalBuffer;
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

        if(pC[i] < 0x1F)
        {
            /*
               if(pC[i] == 0x3)
               {
               Thread *t = Processor::information().getCurrentThread();
               Process *p = t->getParent();
               p->getSubsystem()->kill(Subsystem::Interrupted, t);
               }
               */
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
    String masterName, slaveName;
    masterName.sprintf("pty%c%d", c, i);
    slaveName.sprintf("tty%c%d", c, i);

    ConsoleMasterFile *pMaster = new ConsoleMasterFile(masterName, this);
    ConsoleSlaveFile *pSlave = new ConsoleSlaveFile(slaveName, this);

    pMaster->setOther(pSlave);
    pSlave->setOther(pMaster);

    {
        LockGuard<Spinlock> guard(m_Lock);
        m_Consoles.pushBack(pMaster);
        m_Consoles.pushBack(pSlave);
    }

    NOTICE("Console: registered master " << masterName << " with slave " << slaveName << ".");
}

ConsoleManager::ConsoleManager() :
    m_Consoles(), m_Lock()
{
    // Create all consoles, so we can look them up easily.
    for(size_t i = 0; i < 10; ++i)
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

bool ConsoleManager::registerConsole(String consoleName, RequestQueue *backEnd, uintptr_t param)
{
    FATAL("Console: old-style console creation used!");
    return false;
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

    /// \todo check it is actually a master
    ConsoleMasterFile *pConsole = static_cast<ConsoleMasterFile *>(file);
    if(pConsole->bLocked)
        return false;
    pConsole->bLocked = true;

    return true;
}

void ConsoleManager::unlockConsole(File *file)
{
    if(!isConsole(file))
        return;

    ConsoleMasterFile *pConsole = static_cast<ConsoleMasterFile *>(file);
    pConsole->bLocked = false;
}

bool ConsoleManager::isConsole(File* file)
{
    return (file->getInode() == 0xdeadbeef);
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

#if 0
uint64_t ConsoleFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    // Ensure we're safe to go
    if(!size || !buffer)
        return 0;

#if 1
    // Determine how many bytes to read from the console (can the request be completely
    // fulfilled by our line buffer?)
    if(m_Flags & ConsoleManager::LCookedMode)
    {
        // By default we expect that the line buffer will suffice
        bool bReadFromConsole = false;
        if(m_LineBufferFirstNewline < size)
        {
            size = m_LineBufferFirstNewline;

            /// \bug If more than one line is waiting in the buffer this will not work
            m_LineBufferFirstNewline = ~0UL; // Reading will remove this newline
        }
        if(size > m_LineBufferSize)
        {
            size -= m_LineBufferSize;
            bReadFromConsole = true; // Must read from the console
        }

        // If we're not reading from the console, just copy the buffer
        if(!bReadFromConsole)
        {
            // Copy the buffer
            memcpy(reinterpret_cast<void*>(buffer), m_LineBuffer, size);

            // And now move the buffer over the space we just consumed
            uint64_t nConsumedBytes = m_LineBufferSize - size;
            if(nConsumedBytes) // If zero, the buffer was consumed completely
                memcpy(m_LineBuffer, &m_LineBuffer[size], nConsumedBytes);

            // Reduce the buffer size now
            m_LineBufferSize -= size;

            // All done
            return size;
        }
    }

    // Read what we must from the console
    uint64_t realReadSize = size;
    if(m_Flags & (ConsoleManager::LCookedMode|ConsoleManager::LEcho))
        realReadSize -= m_LineBufferSize;
    char *readBuffer = new char[realReadSize];
    uint64_t nBytes = m_pBackEnd->addRequest(1, CONSOLE_READ, m_Param, realReadSize, reinterpret_cast<uintptr_t>(readBuffer));
    if(!nBytes)
    {
        delete [] readBuffer;
        return 0;
    }

    // Handle temios local modes
    if(m_Flags & (ConsoleManager::LCookedMode|ConsoleManager::LEcho))
    {
        // Whether or not the application buffer has already been filled
        bool bAppBufferComplete = false;

        // Number of bytes added to the application buffer
        uint64_t nBytesAdded = 0;

        // Offset into the destination buffer (for when canonical mode isn't active but ECHO is)
        uint64_t destBuffOffset = 0;

        // Destination buffer, char-style
        char *destBuffer = reinterpret_cast<char*>(buffer);

        // Iterate over the buffer
        while(!bAppBufferComplete)
        {
            for(size_t i = 0; i < nBytes; i++)
            {
                // Handle incoming newline
                if(readBuffer[i] == '\r')
                {
                    // Only echo the newline if we are supposed to
                    if((m_Flags & ConsoleManager::LEcho) || (m_Flags & ConsoleManager::LCookedMode))
                    {
                        m_LineBuffer[m_LineBufferSize++] = '\n';
                        if((m_Flags & ConsoleManager::LEchoNewline) || (m_Flags & ConsoleManager::LEcho))
                            write(location, 1, reinterpret_cast<uintptr_t>(m_LineBuffer) + (m_LineBufferSize-1));

                        if((m_Flags & ConsoleManager::LCookedMode) && !bAppBufferComplete)
                        {
                            // Copy the entire buffer
                            uint64_t realSize = size;
                            if(realSize > m_LineBufferSize)
                                realSize = m_LineBufferSize;
                            if(m_LineBufferFirstNewline < realSize)
                            {
                                realSize = m_LineBufferFirstNewline;
                                m_LineBufferFirstNewline = ~0UL;
                            }
                            memcpy(reinterpret_cast<void*>(buffer + nBytesAdded), m_LineBuffer, realSize);

                            // And now move the buffer over the space we just consumed
                            uint64_t nConsumedBytes = m_LineBufferSize - realSize;
                            if(nConsumedBytes) // If zero, the buffer was consumed completely
                                memcpy(m_LineBuffer, &m_LineBuffer[size], nConsumedBytes);

                            // Reduce the buffer size now
                            nBytesAdded += realSize;
                            m_LineBufferSize -= realSize;

                            // The buffer has been filled!
                            bAppBufferComplete = true;
                        }
                        else if((m_Flags & ConsoleManager::LCookedMode) && (m_LineBufferFirstNewline == ~0UL))
                        {
                            // Application buffer has already been filled, let future runs know where the limit is
                            m_LineBufferFirstNewline = m_LineBufferSize - 1;
                        }
                        else if(!(m_Flags & ConsoleManager::LCookedMode))
                            destBuffer[destBuffOffset++] = readBuffer[i];

                        // Ignore the \n if one is present
                        if(readBuffer[i+1] == '\n')
                            i++;
                    }
                }
                else if(readBuffer[i] == '\x08')
                {
                    if(m_Flags & (ConsoleManager::LCookedMode|ConsoleManager::LEchoErase))
                    {
                        if((m_Flags & ConsoleManager::LCookedMode) && m_LineBufferSize)
                        {
                            char buf[3] = {'\x08', ' ', '\x08'};
                            write(location, 3, reinterpret_cast<uintptr_t>(buf));
                            m_LineBufferSize--;
                        }
                        else if((!(m_Flags & ConsoleManager::LCookedMode)) && destBuffOffset)
                        {
                            char buf[3] = {'\x08', ' ', '\x08'};
                            write(location, 3, reinterpret_cast<uintptr_t>(buf));
                            destBuffOffset--;
                        }
                    }
                }
                else if(readBuffer[i] < 0x1F)
                {
                    /*
                    if(readBuffer[i] == 0x3)
                    {
                        Process *p = Processor::information().getCurrentThread()->getParent();
                        p->getSubsystem()->kill(Subsystem::Interrupted);
                    }
                    */
                }
                else
                {
                    // Write the character to the console
                    /// \todo ISIG handling, special characters
                    if(m_Flags & ConsoleManager::LEcho)
                        write(location, 1, reinterpret_cast<uintptr_t>(readBuffer) + i);

                    // Add to the buffer
                    if(m_Flags & ConsoleManager::LCookedMode)
                        m_LineBuffer[m_LineBufferSize++] = readBuffer[i];
                    else
                        destBuffer[destBuffOffset++] = readBuffer[i];
                }
            }

            // We appear to have hit the top of the line buffer!
            if(m_LineBufferSize >= LINEBUFFER_MAXIMUM)
            {
                // Our best bet is to return early, giving the application what we can of the line buffer
                size_t numBytesToRemove = nBytes > m_LineBufferSize ? nBytes : m_LineBufferSize;

                // Copy the buffer across
                memcpy(reinterpret_cast<void*>(buffer), m_LineBuffer, numBytesToRemove);

                // And now move the buffer over the space we just consumed
                uint64_t nConsumedBytes = m_LineBufferSize - numBytesToRemove;
                if(nConsumedBytes) // If zero, the buffer was consumed completely
                    memcpy(m_LineBuffer, &m_LineBuffer[numBytesToRemove], nConsumedBytes);

                // Reduce the buffer size now
                nBytesAdded += numBytesToRemove;
                m_LineBufferSize -= numBytesToRemove;
                break;
            }

            if((!bAppBufferComplete && (m_Flags & ConsoleManager::LCookedMode)) && (nBytesAdded < size))
            {
                uint64_t nBytes = m_pBackEnd->addRequest(1, CONSOLE_READ, m_Param, size - m_LineBufferSize, reinterpret_cast<uintptr_t>(readBuffer));
                if(!nBytes)
                {
                    delete [] readBuffer;
                    return 0;
                }
            }
            else if((!bAppBufferComplete && (m_Flags & ConsoleManager::LCookedMode)) && m_LineBufferSize == size)
            {
                // We can now fulfill the request
                uint64_t realSize = size;
                if(realSize > m_LineBufferSize)
                    realSize = m_LineBufferSize;
                if(m_LineBufferFirstNewline < realSize)
                {
                    realSize = m_LineBufferFirstNewline;
                    m_LineBufferFirstNewline = ~0UL;
                }
                memcpy(reinterpret_cast<void*>(buffer), m_LineBuffer, realSize);

                // And now move the buffer over the space we just consumed
                uint64_t nConsumedBytes = m_LineBufferSize - realSize;
                if(nConsumedBytes) // If zero, the buffer was consumed completely
                    memcpy(m_LineBuffer, &m_LineBuffer[size], nConsumedBytes);

                // Reduce the buffer size now
                nBytesAdded += realSize;
                m_LineBufferSize -= realSize;

                break;
            }
            else
                break;
        }

        // Fill in the number of bytes we copied into the application buffer
        if(m_Flags & ConsoleManager::LCookedMode)
            nBytes = nBytesAdded;
        else
            nBytes = destBuffOffset;
    }
    else
        memcpy(reinterpret_cast<void*>(buffer), readBuffer, nBytes);

    // Done with the read buffer now
    delete [] readBuffer;

#else
    uint64_t nBytes = m_pBackEnd->addRequest(1, CONSOLE_READ, m_Param, size, buffer);
    if(!nBytes)
    {
        return 0;
    }
#endif

    // Perform input processing.
    char *pC = reinterpret_cast<char*>(buffer);
    for (size_t i = 0; i < nBytes; i++)
    {
        if (m_Flags & ConsoleManager::IStripToSevenBits)
            pC[i] = static_cast<uint8_t>(pC[i]) & 0x7F;

        if(pC[i] < 0x1F)
        {
            /*
            if(pC[i] == 0x3)
            {
                Thread *t = Processor::information().getCurrentThread();
                Process *p = t->getParent();
                p->getSubsystem()->kill(Subsystem::Interrupted, t);
            }
            */
        }

        if (pC[i] == '\n' && (m_Flags & ConsoleManager::IMapNLToCR))
            pC[i] = '\r';
        else if (pC[i] == '\r' && (m_Flags & ConsoleManager::IMapCRToNL))
            pC[i] = '\n';
        else if (pC[i] == '\r' && (m_Flags & ConsoleManager::IIgnoreCR))
        {
            memmove(reinterpret_cast<void*>(buffer+i), reinterpret_cast<void*>(buffer+i+1), nBytes-i-1);
            i--; // Need to process this byte again, its contents have changed.
        }
    }

    return nBytes;
}

uint64_t ConsoleFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    char *tmpBuff = new char[size];
    size_t realSize = size;

    // Post-process output if enabled.
    if(m_Flags & (ConsoleManager::OPostProcess))
    {
        char *pC = reinterpret_cast<char*>(buffer);
        for (size_t i = 0, j = 0; j < size; j++)
        {
            bool bInsert = true;

            // OCRNL: Map CR to NL on output
            if (pC[j] == '\r' && (m_Flags & ConsoleManager::OMapCRToNL))
            {
                tmpBuff[i++] = '\n';
                continue;
            }

            // ONLCR: Map NL to CR-NL on output
            else if (pC[j] == '\n' && (m_Flags & ConsoleManager::OMapNLToCRNL))
            {
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
            if(pC[j] == '\n' && (m_Flags & ConsoleManager::ONLCausesCR))
            {
                /// \note The documentation for ONLRET is total rubbish... It *seems* just like ONLCR!
                ///       Or is it supposed to map '\n' to '\r'? Other documents seem to point to a
                ///       newline "key" instead of "character" - what a mess!
                ///       So for now, I'm going to hope this doesn't break anything, but it's just
                ///       going to make newlines a line feed.
            }

            if(bInsert)
            {
                tmpBuff[i++] = pC[j];
            }
        }
    }
    else
        memcpy(tmpBuff, reinterpret_cast<char*>(buffer), size);

    /* Async */
    uint64_t nBytes = m_pBackEnd->addRequest(1, CONSOLE_WRITE, m_Param, realSize, reinterpret_cast<uint64_t>(tmpBuff));
    delete [] tmpBuff;

    return size;
}

void ConsoleFile::truncate()
{
    m_pBackEnd->addRequest(1, CONSOLE_REFRESH, m_Param);
}
#endif

int ConsoleManager::getCols(File* file)
{
    if(!file)
        return 0;
    return 80;
}

int ConsoleManager::getRows(File* file)
{
    if(!file)
        return 0;
    return 25;
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

static void initConsole()
{
}

static void destroyConsole()
{
}

MODULE_INFO("console", &initConsole, &destroyConsole, "vfs");
