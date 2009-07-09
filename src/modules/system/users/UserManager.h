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

#include <processor/types.h>
#include <utilities/Tree.h>

#include "User.h"
#include "Group.h"

/** The user manager - this allows lookups of users and groups, and also
    parses the initial file "root:/config/users". */
class UserManager
{
public:
  /** Get the singleton instance. */
  static UserManager &instance() {return m_Instance;}

  /** Reads in the file "root:/config/users". */
  void initialise();

  /** Look up a user by ID. */
  User *getUser(size_t id);
  /** Look up a user by name. */
  User *getUser(String name);

  /** Look up a group by ID. */
  Group *getGroup(size_t id);
  /** Look up a group by name. */
  Group *getGroup(String name);

private:
  /** Singleton class - default constructor hidden. */
  UserManager();
  ~UserManager();
  UserManager(const UserManager&);
  UserManager &operator = (const UserManager&);

  void initialiseUsers();
  void initialiseGroups();
  void addUser(size_t uid, String username, String fullName, String group, String home, String shell, String password);
  void addGroup(size_t gid, String name);

  /** Dictionary of users, indexed by ID. */
  Tree<size_t, User*> m_Users;
  /** Dictionary of groups, indexed by ID. */
  Tree<size_t, Group*> m_Groups;

  static UserManager m_Instance;
};
