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

#include "UserManager.h"
#include <Module.h>
#include <vfs/VFS.h>
#include <utilities/utility.h>
#include <process/Process.h>

UserManager UserManager::m_Instance;

UserManager::UserManager() :
  m_Users(), m_Groups()
{
}

UserManager::~UserManager()
{
}

void UserManager::initialiseGroups()
{
  // Find and parse the config file.
  File *pFile = VFS::instance().find(String("root:/config/groups"));
  if (!pFile || !pFile->isValid())
  {
    ERROR("USERS: Unable to open file root:/config/groups.");
    return;
  }

  // Else, we can read it in and get cracking.
  char *pBuffer = new char[pFile->getSize()+1];
  if (pFile->read(0, pFile->getSize(), reinterpret_cast<uintptr_t>(pBuffer)) != pFile->getSize())
  {
    WARNING("USERS: Read length of file 'groups' differs from as advertised!");
    return;
  }

  // Start the scan process.
  char *pStr = 0;
  enum GroupState
  {
    Gid = 0,
    Name,
    Whitespace
  };

  GroupState state = Whitespace;
  GroupState nextstate = Gid;
  char *pStrings[2];
  size_t i = 0;
  while (i < pFile->getSize())
  {
    // For every line...

    // Check for a comment.
    if (pBuffer[i] == '#')
    {
      while (pBuffer[i] != '\n') i++;
      continue;
    }

    // Blank lines...
    if (pBuffer[i] == '\n')
    {
      i++;
      continue;
    }

    state = Whitespace;
    nextstate = Gid;
    bool bStop = false;

    while (!bStop)
    {
      if (i == pFile->getSize()) pBuffer[i] = '\n';
      switch (pBuffer[i])
      {
        case '\n':
          // Command finished.
          pBuffer[i] = '\0';
          addGroup(strtoul(pStrings[Gid], 0, 10), String(pStrings[Name]));
          bStop = true;
          break;

        case ';':
          if (state != Whitespace)
          {
            pBuffer[i] = '\0'; // Null-terminate current str.
            state = Whitespace;
          }
          break;

        default:
          if (state == Whitespace)
          {
            state = nextstate;
            nextstate = static_cast<GroupState>(state+1);
            if (state == Whitespace)
            {
              WARNING("USER: Malformed line in 'groups'.");
              bStop = true;
              break;
            }
            else
              pStrings[state] = &pBuffer[i];
          }
      }
      i++;
    }
  }
}

void UserManager::initialiseUsers()
{
  // Find and parse the config file.
  File *pFile = VFS::instance().find(String("root:/config/users"));

  if (!pFile || !pFile->isValid())
  {
    ERROR("USERS: Unable to open file root:/config/users.");
    return;
  }

  // Else, we can read it in and get cracking.
  char *pBuffer = new char[pFile->getSize()+1];
  if (pFile->read(0, pFile->getSize(), reinterpret_cast<uintptr_t>(pBuffer)) != pFile->getSize())
  {
    WARNING("USERS: Read length of file differs from as advertised!");
    return;
  }

  // Start the scan process.
  char *pStr = 0;
  enum UserState
  {
    Uid = 0,
    Username,
    Fullname,
    Group,
    Home,
    Shell,
    Password,
    Whitespace
  };

  UserState state = Whitespace;
  UserState nextstate = Uid;
  char *pStrings[7];
  size_t i = 0;
  while (i < pFile->getSize())
  {
    // For every line...

    // Check for a comment.
    if (pBuffer[i] == '#')
    {
      while (pBuffer[i] != '\n') i++;
      continue;
    }

    // Blank lines...
    if (pBuffer[i] == '\n')
    {
      i++;
      continue;
    }

    state = Whitespace;
    nextstate = Uid;
    bool bStop = false;
    while (!bStop)
    {
      if (i == pFile->getSize()) pBuffer[i] = '\n';
      switch (pBuffer[i])
      {
        case '\n':
          // Command finished.
          pBuffer[i] = '\0';
          // Add user.
          addUser(strtoul(pStrings[Uid],0,10), String(pStrings[Username]),
                  String(pStrings[Fullname]), String(pStrings[Group]),
                  String(pStrings[Home]), String(pStrings[Shell]),
                  String(pStrings[Password]));
          bStop = true;
          break;

        case ';':
          if (state != Whitespace)
          {
            pBuffer[i] = '\0'; // Null-terminate current str.
            state = Whitespace;
          }
          break;

        default:
          if (state == Whitespace)
          {
            state = nextstate;
            nextstate = static_cast<UserState>(state+1);
            if (state == Whitespace)
            {
              WARNING("USER: Malformed line in 'users'.");
              bStop = true;
              break;
            }
            else
              pStrings[state] = &pBuffer[i];
          }
      }
      i++;
    }
  }
}

User *UserManager::getUser(size_t id)
{
  return m_Users.lookup(id);
}

User *UserManager::getUser(String name)
{
  for (Tree<size_t,User*>::Iterator it = m_Users.begin();
       it != m_Users.end();
       it++)
  {
    User *pU = reinterpret_cast<User*> (it.value());
    if ( pU->getUsername() == name )
      return pU;
  }
  return 0;
}

Group *UserManager::getGroup(size_t id)
{
  return m_Groups.lookup(id);
}

Group *UserManager::getGroup(String name)
{
  for (Tree<size_t,Group*>::Iterator it = m_Groups.begin();
       it != m_Groups.end();
       it++)
  {
    Group *pG = reinterpret_cast<Group*> (it.value());
    if ( pG->getName() == name )
      return pG;
  }
  return 0;
}

void UserManager::addUser(size_t uid, String username, String fullName, String group, String home, String shell, String password)
{
  // Check for duplicates.
  if (getUser(uid))
  {
    WARNING("USERS: Not inserting user '" << username << "' with uid " << uid << ": duplicate.");
    return;
  }

  // Check that the given group exists.
  Group *pGroup = getGroup(group);
  if (!pGroup)
  {
    WARNING("USERS: Not inserting user '" << username << "': group '" << group << "' does not exist.");
    return;
  }

  NOTICE("USERS: Adding user {" << uid << ",'" << username << "','" << fullName << "'}");
  User *pUser = new User(uid, username, fullName, pGroup, home, shell, password);
  pGroup->join(pUser);
  m_Users.insert(uid, pUser);
}

void UserManager::addGroup(size_t gid, String name)
{
  // Check for duplicates.
  if (getGroup(gid))
  {
    WARNING("USERS: Not inserting group '" << name << "' with gid " << gid << ": duplicate.");
    return;
  }

  NOTICE("USERS: Adding group {" << gid << ",'" << name << "'}");
  Group *pGroup = new Group(gid, name);
  m_Groups.insert(gid, pGroup);
}

void UserManager::initialise()
{
  initialiseGroups();
  initialiseUsers();

  User *pUser = getUser(0);
  if (!pUser)
  {
    FATAL("USERS: Unable to set default user (no such user for uid 0)");
    return;
  }
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  
  pProcess->setUser(pUser);
  pProcess->setGroup(pUser->getDefaultGroup());
}

void init()
{
}

void destroy()
{
}

MODULE_NAME("users");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
MODULE_DEPENDS("VFS");
