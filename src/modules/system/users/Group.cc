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

#include "Group.h"
#include "User.h"

Group::Group(size_t gid, String name) :
  m_Gid(gid), m_Name(name), m_Users()
{
}

Group::~Group()
{
}

void Group::join(User *pUser)
{
  m_Users.pushBack(pUser);
}

void Group::leave(User *pUser)
{
  for (List<User*>::Iterator it = m_Users.begin();
       it != m_Users.end();
       it++)
  {
    if (*it == pUser)
    {
      m_Users.erase(it);
      return;
    }
  }
}

bool Group::isMember(User *pUser)
{
  for (List<User*>::Iterator it = m_Users.begin();
       it != m_Users.end();
       it++)
  {
    if (*it == pUser)
      return true;
  }
  return false;
}
