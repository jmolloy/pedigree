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
        m_Name(), m_pBackEnd(0), m_Param(0), m_Flags(DEFAULT_FLAGS)
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

uint64_t ConsoleManager::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    /// \todo Sanity checking.
    ConsoleFile *file = reinterpret_cast<ConsoleFile*>(pFile);
    uint64_t nBytes =  file->m_pBackEnd->addRequest(CONSOLE_READ, file->m_Param, size, buffer);

    // Perform cooked mode processing if required.
#if 0
    if (file->m_Flags & (LCookedMode|LEcho))
    {
        char *pC = reinterpret_cast<char*>(buffer);
        for (size_t i = 0; i < nBytes; i++)
        {
            // If Echo or EchoNewline
            if (pC[i] == '\n' && ((file->m_Flags & LEcho) ||
                                  ((file->m_Flags & LEchoNewline) && file->m_Flags & LCookedMode)))
                write(file, location, 1, buffer+i);

            else if (pC[i] == '\x08' && (file->m_Flags & LEchoErase) &&
                     (file->m_Flags & LCookedMode))
            {
                // If EchoErase in cooked mode, we must not only echo the
                // erase, but also chomp the erase character itself.
                char buf[3] = {'\x08', ' ', '\x08'};
                write(file, location, 3, reinterpret_cast<uintptr_t>(buf));
                memmove(reinterpret_cast<void*>(buffer+i), reinterpret_cast<void*>(buffer+i+1), nBytes-i-1);
                i--; // Need to process this byte again, its contents have changed.
                nBytes--;
            }

            else if (file->m_Flags & LEcho)
                write(file, location, nBytes, buffer);
        }
    }
#endif
    // Perform input processing.
    char *pC = reinterpret_cast<char*>(buffer);
    for (size_t i = 0; i < nBytes; i++)
    {
        /// \note Turned this into a C++-style cast... I have no idea what this achieves. - Matt
        if (file->m_Flags & IStripToSevenBits)
            pC[i] = static_cast<uint8_t>(pC[i]) & 0x7F;

        if (pC[i] == '\n' && (file->m_Flags & IMapNLToCR))
            pC[i] = '\r';
        else if (pC[i] == '\r' && (file->m_Flags & IMapCRToNL))
            pC[i] = '\n';
        else if (pC[i] == '\r' && (file->m_Flags & IIgnoreCR))
        {
            memmove(reinterpret_cast<void*>(buffer+i), reinterpret_cast<void*>(buffer+i+1), nBytes-i-1);
            i--; // Need to process this byte again, its contents have changed.
        }
    }

    return nBytes;
}

uint64_t ConsoleManager::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    /// \todo Sanity checking.
    ConsoleFile *file = reinterpret_cast<ConsoleFile*>(pFile);

    // Post-process output if enabled.
    if ((file->m_Flags & OPostProcess))
    {
        char *pC = reinterpret_cast<char*>(buffer);
        for (size_t i = 0; i < size; i++)
        {
            if (pC[i] == '\r' && (file->m_Flags & OMapCRToNL))
                pC[i] = '\n';
            else if (pC[i] == '\n' && (file->m_Flags & OMapNLToCR))
                pC[i] = '\r';

            // Lastly, after both mappings above have been done, we can
            // check if NL should cause a CR as well.
            if (pC[i] == '\n' && (file->m_Flags & ONLCausesNewline) == 0)
                pC[i] = '\xB'; // Use 'VT' - vertical tab.
        }
    }

    char *newbuf = new char[size];
    memcpy(newbuf, reinterpret_cast<void*>(buffer), size);
    uint64_t nBytes = file->m_pBackEnd->addAsyncRequest(CONSOLE_WRITE, file->m_Param, size, reinterpret_cast<uint64_t>(newbuf));

    return size;
}

int ConsoleManager::getCols(File* file)
{
    /// \todo Sanity checking.
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    return static_cast<int>(pFile->m_pBackEnd->addRequest(CONSOLE_GETCOLS, pFile->m_Param));
}

int ConsoleManager::getRows(File* file)
{
    /// \todo Sanity checking.
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    return static_cast<int>(pFile->m_pBackEnd->addRequest(CONSOLE_GETROWS, pFile->m_Param));
}

bool ConsoleManager::hasDataAvailable(File* file)
{
    /// \todo Sanity checking.
    ConsoleFile *pFile = reinterpret_cast<ConsoleFile*>(file);
    return static_cast<bool>(pFile->m_pBackEnd->addRequest(CONSOLE_DATA_AVAILABLE, pFile->m_Param));
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

