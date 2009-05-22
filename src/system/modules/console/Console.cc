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

ConsoleManager ConsoleManager::m_Instance;

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
  Console *pConsole = new Console;

  pConsole->name = consoleName;
  pConsole->backEnd = backEnd;
  pConsole->param = param;

  m_Consoles.pushBack(pConsole);

  return true;
}

File* ConsoleManager::getConsole(String consoleName)
{
  /// \todo Thread safety.
  for (size_t i = 0; i < m_Consoles.count(); i++)
  {
    Console *pC = m_Consoles[i];
    if (!strcmp(static_cast<const char*>(pC->name), static_cast<const char*>(consoleName)))
    {
      return new File(pC->name, 0, 0, 0, i+0xdeadbe00, this, 0, 0);
    }
  }
  // Error - not found.
  return 0;
}

bool ConsoleManager::isConsole(File* file)
{
  return ((file->getInode()&0xFFFFFF00) == 0xdeadbe00);
}

void ConsoleManager::setAttributes(File* file, bool echo, bool echoNewlines, bool echoBackspace, bool nlCausesCr, bool mapNlToCrIn, bool mapCrToNlIn)
{
  // \todo Sanity checking.
  Console *pC = m_Consoles[file->getInode()-0xdeadbe00];
//  pC->backEnd->addRequest(CONSOLE_SETATTR, pC->param, echo, echoNewlines, echoBackspace, nlCausesCr, mapNlToCrIn, mapCrToNlIn);
}

void ConsoleManager::getAttributes(File* file, bool *echo, bool *echoNewlines, bool *echoBackspace, bool *nlCausesCr, bool *mapNlToCrIn, bool *mapCrToNlIn)
{
  // \todo Sanity checking.
  Console *pC = m_Consoles[file->getInode()-0xdeadbe00];
//  pC->backEnd->addRequest(CONSOLE_GETATTR, pC->param, reinterpret_cast<uint64_t>(echo), reinterpret_cast<uint64_t>(echoNewlines), reinterpret_cast<uint64_t>(echoBackspace), reinterpret_cast<uint64_t>(nlCausesCr), reinterpret_cast<uint64_t>(mapNlToCrIn), reinterpret_cast<uint64_t>(mapCrToNlIn));
}

uint64_t ConsoleManager::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  /// \todo Sanity checking.
  Console *pC = m_Consoles[pFile->getInode()-0xdeadbe00];
  return pC->backEnd->addRequest(CONSOLE_READ, pC->param, size, buffer);
}

uint64_t ConsoleManager::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  /// \todo Sanity checking.
  Console *pC = m_Consoles[pFile->getInode()-0xdeadbe00];
  return pC->backEnd->addRequest(CONSOLE_WRITE, pC->param, size, buffer);
}

int ConsoleManager::getCols(File* file)
{
  /// \todo Sanity checking.
  Console *pC = m_Consoles[file->getInode()-0xdeadbe00];
  return static_cast<int>(pC->backEnd->addRequest(CONSOLE_GETCOLS));
}

int ConsoleManager::getRows(File* file)
{
  /// \todo Sanity checking.
  Console *pC = m_Consoles[file->getInode()-0xdeadbe00];
  return static_cast<int>(pC->backEnd->addRequest(CONSOLE_GETROWS));
}

bool ConsoleManager::hasDataAvailable(File* file)
{
  /// \todo Sanity checking.
  Console *pC = m_Consoles[file->getInode()-0xdeadbe00];
  return static_cast<bool>(pC->backEnd->addRequest(CONSOLE_DATA_AVAILABLE));
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

