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

#ifndef GROUP_H
#define GROUP_H

#include <processor/types.h>
#include <utilities/String.h>
#include <utilities/List.h>

class User;

/** Defines the properties of a Group on the system.  */
class Group
{
public:
  /** Constructor.
      \param gid System-wide unique group ID.
      \param name Group name. */
  Group(size_t gid, String name);
  virtual ~Group();

  /** Adds a user. */
  void join(User *pUser);

  /** Removes a user. */
  void leave(User *pUser);

  /** Queries user membership. */
  bool isMember(User *pUser);

  /** Returns the GID. */
  size_t getId()
  {
    return m_Gid;
  }
  /** Returns the group name. */
  String getName()
  {
    return m_Name;
  }

private:
  /** It doesn't make sense for a Group to have public default or copy constructors. */
  Group();
  Group(const Group&);
  Group &operator = (const Group&);

  /** Group ID. */
  size_t m_Gid;
  /** Name. */
  String m_Name;
  /** Group contents. */
  List<User*> m_Users;
};

#endif

