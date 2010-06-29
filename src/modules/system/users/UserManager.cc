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
#include <utilities/utility.h>
#include <process/Process.h>
#include <processor/Processor.h>
#include <config/Config.h>

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
  Config::Result *pResult = Config::instance().query("select * from groups");
  if (!pResult->succeeded())
      ERROR("Error: " << pResult->errorMessage());

  for (size_t i = 0; i < pResult->rows();i++)
  {
    uint32_t gid;
    String name;

    gid = pResult->getNum(i, "gid") - 1;
    name = pResult->getStr(i, "name");

    addGroup(gid, name);
  }

  delete pResult;
}

void UserManager::initialiseUsers()
{
  Config::Result *pResult = Config::instance().query("select * from users");
  if (!pResult->succeeded())
      ERROR("Error: " << pResult->errorMessage());

  for (size_t i = 0; i < pResult->rows();i++)
  {
    uint32_t uid;
    String username, fullname, groupname, homedir, shell, password;

    uid = pResult->getNum(i, "uid") - 1;
    username = pResult->getStr(i, "username");
    fullname = pResult->getStr(i, "fullname");
    groupname = pResult->getStr(i, "groupname");
    homedir = pResult->getStr(i, "homedir");
    shell = pResult->getStr(i, "shell");
    password = pResult->getStr(i, "password");

    addUser(uid, username, fullname, groupname, homedir, shell, password);
  }

  delete pResult;
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
#ifdef INSTALLER
  addGroup(0, String("root"));
  addUser(0, String("root"), String("root"), String("root"), String("/"), String("/applications/bash"), String(""));
#else
  initialiseGroups();
  initialiseUsers();
#endif

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

static void init()
{
}

static void destroy()
{
}

MODULE_INFO("users", &init, &destroy);
