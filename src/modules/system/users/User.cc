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

#include "User.h"
#include "Group.h"
#include <process/Process.h>
#include <processor/Processor.h>

User::User(size_t uid, String username, String fullName, Group *pGroup, String home, String shell, String password):
  m_Uid(uid), m_Username(username), m_FullName(fullName), m_pDefaultGroup(pGroup),
  m_Home(home), m_Shell(shell), m_Password(password), m_Groups()
{
}

User::~User()
{
}

void User::join(Group *pGroup)
{
  m_Groups.pushBack(pGroup);
}

void User::leave(Group *pGroup)
{
  for (List<Group*>::Iterator it = m_Groups.begin();
       it != m_Groups.end();
       it++)
  {
    if (*it == pGroup)
    {
      m_Groups.erase(it);
      return;
    }
  }
}

bool User::isMember(Group *pGroup)
{
  if (pGroup == m_pDefaultGroup) return true;
  for (List<Group*>::Iterator it = m_Groups.begin();
       it != m_Groups.end();
       it++)
  {
    if (*it == pGroup)
      return true;
  }
  return false;
}

bool User::login(String password)
{
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  
  if (password == m_Password)
  {
    pProcess->setUser(this);
    pProcess->setGroup(m_pDefaultGroup);
    return true;
  }
  else return false;
}
