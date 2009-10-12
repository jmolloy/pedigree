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
    m_Name(), m_pBackEnd(0), m_Param(0), m_Flags(DEFAULT_FLAGS),
    m_LineBuffer(), m_LineBufferSize(0), m_LineBufferFirstNewline(~0UL)
{
}

int ConsoleFile::select(bool bWriting, int timeout)
{
    bool ret = false;
    if(timeout == 0)
        ret = ConsoleManager::instance().hasDataAvailable(this);
    else
        while(!(ret = ConsoleManager::instance().hasDataAvailable(this))) Scheduler::instance().yield();
    return (ret ? 1 : 0);
}

ConsoleManager::ConsoleManager() :
    m_Consoles()
{
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
    ConsoleFile *pConsole = new ConsoleFile(consoleName, this);

    pConsole->m_Name = consoleName;
    pConsole->m_pBackEnd = backEnd;
    pConsole->m_Param = param;

    m_Consoles.pushBack(pConsole);

    return true;
}

File* ConsoleManager::getConsole(String consoleName)
{
    /// \todo Thread safety.
    for (size_t i = 0; i < m_Consoles.count(); i++)
    {
        ConsoleFile *pC = m_Consoles[i];
        if (!strcmp(static_cast<const char*>(pC->m_Name), static_cast<const char*>(consoleName)))
        {
            return pC;
        }
    }
    // Error - not found.
    return 0;
}

ConsoleFile *ConsoleManager::getConsoleFile(RequestQueue *pBackend)
{
    /// \todo Thread safety.
    for (size_t i = 0; i < m_Consoles.count(); i++)
    {
        ConsoleFile *pC = m_Consoles[i];
        if (pC->m_pBackEnd == pBackend)
        {
            return pC;
        }
    }
    // Error - not found.
    return 0;
}

bool ConsoleManager::isConsole(File* file)
{
    return (file->getInode() == 0xdeadbeef);
}

void ConsoleManager::setAttributes(File* file, size_t flags)
{
    // \todo Sanity checking.
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    pFile->m_Flags = flags;
}

void ConsoleManager::getAttributes(File* file, size_t *flags)
{
    // \todo Sanity checking.
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    *flags = pFile->m_Flags;
}

uint64_t ConsoleFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    /// \todo Sanity checking.

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
    char *readBuffer = new char[size];
    uint64_t nBytes = m_pBackEnd->addRequest(1, CONSOLE_READ, m_Param, size - m_LineBufferSize, reinterpret_cast<uintptr_t>(readBuffer));
    if(!nBytes)
    {
        delete [] readBuffer;
        return 0;
    }

    // Handle temios local modes
    /// \todo Handle overflow of the line buffer
    if(m_Flags & (ConsoleManager::LCookedMode|ConsoleManager::LEcho))
    {
        // Whether or not the application buffer has already been filled
        bool bAppBufferComplete = false;

        // Number of bytes added to the application buffer
        uint64_t nBytesAdded = 0;

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
                            memcpy(reinterpret_cast<void*>(buffer), m_LineBuffer, realSize);

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

                        // Ignore the \n if one is present
                        if(readBuffer[i+1] == '\n')
                            i++;
                    }
                }
                else if(readBuffer[i] == '\x08')
                {
                    if(m_Flags & (ConsoleManager::LCookedMode|ConsoleManager::LEchoErase))
                    {
                        char buf[3] = {'\x08', ' ', '\x08'};
                        write(location, 3, reinterpret_cast<uintptr_t>(buf));
                        m_LineBufferSize--;
                    }
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
                }
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
            else
                break;
        }

        // Fill in the number of bytes we copied into the application buffer
        nBytes = nBytesAdded;
    }
    else
        memcpy(reinterpret_cast<void*>(buffer), readBuffer, nBytes);

    // Done with the read buffer now
    delete [] readBuffer;

    // Perform input processing.
    char *pC = reinterpret_cast<char*>(buffer);
    for (size_t i = 0; i < nBytes; i++)
    {
        if (m_Flags & ConsoleManager::IStripToSevenBits)
            pC[i] = static_cast<uint8_t>(pC[i]) & 0x7F;

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
    // Post-process output if enabled.
    if (m_Flags & (ConsoleManager::LCookedMode|ConsoleManager::LEcho))
    {
        char *pC = reinterpret_cast<char*>(buffer);
        for (size_t i = 0; i < size; i++)
        {
            if (pC[i] == '\r' && (m_Flags & ConsoleManager::OMapCRToNL))
                pC[i] = '\n';
            else if (pC[i] == '\n' && (m_Flags & ConsoleManager::OMapNLToCRNL))
                // We don't make the distinction between '\r\n' and '\n'.
                pC[i] = '\n';

            // Lastly, after both mappings above have been done, we can
            // check if NL should cause a CR as well.
            else if (pC[i] == '\n' && !(m_Flags & ConsoleManager::ONLCausesCR))
                pC[i] = '\xB'; // Use 'VT' - vertical tab.
        }
    }

    char *newbuf = new char[size];
    memcpy(newbuf, reinterpret_cast<void*>(buffer), size);
    uint64_t nBytes = m_pBackEnd->addAsyncRequest(1, CONSOLE_WRITE, m_Param, size, reinterpret_cast<uint64_t>(newbuf));

    return size;
}

int ConsoleManager::getCols(File* file)
{
    /// \todo Sanity checking.
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    return static_cast<int>(pFile->m_pBackEnd->addRequest(1, CONSOLE_GETCOLS, pFile->m_Param));
}

int ConsoleManager::getRows(File* file)
{
    /// \todo Sanity checking.
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    return static_cast<int>(pFile->m_pBackEnd->addRequest(1, CONSOLE_GETROWS, pFile->m_Param));
}

bool ConsoleManager::hasDataAvailable(File* file)
{
    /// \todo Sanity checking.
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);

    // If there's data in the line buffer, we're able to read
    if(pFile->m_Flags & ConsoleManager::LCookedMode)
        if(pFile->m_LineBufferSize)
            return true;

    // But otherwise, check via more conventional means
    return static_cast<bool>(pFile->m_pBackEnd->addRequest(1,CONSOLE_DATA_AVAILABLE, pFile->m_Param));
}

void initConsole()
{
}

void destroyConsole()
{
}

MODULE_NAME("console");
MODULE_ENTRY(&initConsole);
MODULE_EXIT(&destroyConsole);
MODULE_DEPENDS(0);

